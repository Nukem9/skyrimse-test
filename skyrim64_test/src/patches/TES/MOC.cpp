#include <DirectXMath.h>
#include "../rendering/common.h"
#include "../../common.h"
#include "BSShader/BSShaderUtil.h"
#include "BSGraphicsRenderer.h"
#include "BSBatchRenderer.h"
#include "MOC.h"

#include "NiMain/BSMultiBoundNode.h"

#include "NiMain/NiNode.h"
#include "NiMain/NiCamera.h"
#include <DirectXCollision.h>
#include <smmintrin.h>
using namespace DirectX;

#include "../../../MaskedOcclusionCulling/CullingThreadpool.h"
#include "../../../MaskedOcclusionCulling/MaskedOcclusionCulling.h"

#include "../../../meshoptimizer/src/meshoptimizer.h"

#include "../../../tbb2018/concurrent_queue.h"

const int MOC_WIDTH = 1280;
const int MOC_HEIGHT = 720;

extern ID3D11Texture2D *g_OcclusionTexture;
extern ID3D11ShaderResourceView *g_OcclusionTextureSRV;

static uint8_t *mpGPUDepthBuf = (uint8_t *)_aligned_malloc(sizeof(char) * ((MOC_WIDTH * MOC_HEIGHT * 4 + 31) & 0xFFFFFFE0), 32);

namespace MOC
{
	struct IndexPair
	{
		uint32_t *Data;
		uint32_t Count;
	};

	std::unordered_map<void *, float *> m_VertMap;
	std::unordered_map<void *, IndexPair> m_IndexMap;

	uint32_t *ConvertIndices(const void *Input, uint32_t Count, uint32_t MaxVertexCount)
	{
		const uint16_t *in = (uint16_t *)Input;
		uint32_t *out = new uint32_t[Count];

		for (uint32_t i = 0; i < Count; i++)
		{
			out[i] = in[i];

			if (MaxVertexCount != 0)
			{
				if (out[i] >= MaxVertexCount)
					__debugbreak();
			}
		}

		return out;
	}

	float *ConvertVerts(const void *Input, uint32_t Count, uint32_t ByteStride)
	{
		uintptr_t data = (uintptr_t)(new float[Count * 4]);
		uintptr_t base = data;
		uintptr_t in = (uintptr_t)Input;

		for (uint32_t i = 0; i < Count; i++)
		{
			//
			// 16+X bytes    16 bytes
			// XYZW[DATA] -> XY-W
			//
			*(float *)(data + 0) = *(float *)(in + 0);
			*(float *)(data + 4) = *(float *)(in + 4);
			*(float *)(data + 8) = 1.0f;
			*(float *)(data + 12) = *(float *)(in + 8);

			for (int j = 0; j < 4; j++)
			{
				if (std::isnan(*(float *)(data + 4 * j)) ||
					!std::isfinite(*(float *)(data + 4 * j)) ||
					*(float *)(data + 4 * j) > 50000.0f ||
					*(float *)(data + 4 * j) < -50000.0f)
					__debugbreak();
			}

			data += 4 * sizeof(float);
			in += ByteStride;
		}

		return (float *)base;
	}

	SRWLOCK vertLock = SRWLOCK_INIT;
	void GetCachedVerticesAndIndices(BSGeometry *Geometry, IndexPair *Indices, float **Vertices)
	{
		void *dataPtr;

		const void *indexData;
		uint32_t indexCount;

		const void *vertexData;
		uint32_t vertexCount;
		uint32_t vertexStride;

		AcquireSRWLockExclusive(&vertLock);

		if (Geometry->QType() == GEOMETRY_TYPE_TRISHAPE)
		{
			auto triShape = static_cast<BSTriShape *>(Geometry);
			auto rendererData = reinterpret_cast<BSGraphics::TriShape *>(triShape->QRendererData());
			dataPtr = rendererData;

			auto itr = m_IndexMap.find(rendererData);
			auto itr2 = m_VertMap.find(rendererData);

			if (itr != m_IndexMap.end())
				*Indices = itr->second;
			else
				Indices->Data = nullptr;

			if (itr2 != m_VertMap.end())
				*Vertices = itr2->second;
			else
				*Vertices = nullptr;

			indexData = rendererData->m_RawIndexData;
			indexCount = triShape->m_TriangleCount * 3;

			vertexData = rendererData->m_RawVertexData;
			vertexCount = triShape->m_VertexCount;
			vertexStride = BSGeometry::CalculateVertexSize(rendererData->m_VertexDesc);
		}
		else if (Geometry->QType() == GEOMETRY_TYPE_DYNAMIC_TRISHAPE)
		{
			AssertMsg(false, "BSGraphics::DynamicTriShape -> indices are always a nullptr");
		}
		else
		{
			__debugbreak();
		}

		// If one wasn't found, it needs conversion
		if (!Indices->Data)
		{
			IndexPair p;
			p.Data = ConvertIndices(indexData, indexCount, vertexCount);
			p.Count = indexCount;

			if (indexCount > 300)
				p.Count = meshopt_simplify(p.Data, p.Data, indexCount, (const float *)vertexData, vertexCount, vertexStride, 297);// Target 99 triangles

			m_IndexMap.insert_or_assign(dataPtr, p);
			*Indices = p;
		}

		if (!*Vertices)
		{
			*Vertices = ConvertVerts(vertexData, vertexCount, vertexStride);
			m_VertMap.insert_or_assign(dataPtr, *Vertices);
		}

		ReleaseSRWLockExclusive(&vertLock);
	}

	void RemoveCachedVerticesAndIndices(void *RendererData)
	{
		AcquireSRWLockExclusive(&vertLock);

		auto indItr = m_IndexMap.find(RendererData);
		auto vertItr = m_VertMap.find(RendererData);

		if (indItr != m_IndexMap.end())
		{
			delete[] indItr->second.Data;
			m_IndexMap.erase(indItr);
		}

		if (vertItr != m_VertMap.end())
		{
			delete[] vertItr->second;
			m_VertMap.erase(vertItr);
		}

		ReleaseSRWLockExclusive(&vertLock);
	}

	void DepthColorize(const float *FloatData, uint8_t *OutColorArray)
	{
		int w = MOC_WIDTH;
		int h = MOC_HEIGHT;

		// Find min/max w coordinate (discard cleared pixels)
		float minW = FLT_MAX, maxW = 0.0f;
		for (int i = 0; i < w * h; i++)
		{
			if (FloatData[i] > 0.0f)
			{
				minW = min(minW, FloatData[i]);
				maxW = max(maxW, FloatData[i]);
			}
		}

		// Tone map depth values
		for (int i = 0; i < w * h; i++)
		{
			int intensity = 0;

			if (FloatData[i] > 0)
				intensity = (unsigned char)(223.0*(FloatData[i] - minW) / (maxW - minW) + 32.0);

			OutColorArray[i * 4 + 0] = intensity;
			OutColorArray[i * 4 + 1] = intensity;
			OutColorArray[i * 4 + 2] = intensity;
			OutColorArray[i * 4 + 3] = intensity;
		}
	}

	bool dohack = false;

	XMMATRIX MyView;
	XMMATRIX MyProj;
	XMMATRIX MyViewProj;
	NiPoint3 MyPosAdjust;

	const int AABB_VERTICES = 8;
	static const UINT sBBxInd[AABB_VERTICES] = { 1, 0, 0, 1, 1, 1, 0, 0 };
	static const UINT sBByInd[AABB_VERTICES] = { 1, 1, 1, 1, 0, 0, 0, 0 };
	static const UINT sBBzInd[AABB_VERTICES] = { 1, 1, 0, 0, 0, 1, 1, 0 };

	const NiCamera *testCam = nullptr;

	enum CullType
	{
		CULL_COLLECT,
		CULL_RENDER_GEOMETRY,
		CULL_FLUSH,
	};

	struct CullPacket
	{
		BSGeometry *Geometry;
		CullType Type;
	};

	std::array<MaskedOcclusionCulling *, 2> MocList;
	std::atomic_bool ThreadWorking[2];
	std::atomic<MaskedOcclusionCulling *> FinalBuffer;

	tbb::concurrent_queue<CullPacket> PendingPackets;
	void TraverseSceneGraph();

	DWORD WINAPI CullThread(LPVOID Arg)
	{
		uint32_t threadIndex = (uint32_t)Arg;

		MaskedOcclusionCulling *moc = MaskedOcclusionCulling::Create();
		moc->SetResolution(MOC_WIDTH, MOC_HEIGHT);
		moc->ClearBuffer();

		MocList[threadIndex] = moc;
		ThreadWorking[threadIndex].store(false);

		CullPacket p;
		int idleCount = 0;

		while (true)
		{
			if (!PendingPackets.try_pop(p))
			{
				if (++idleCount <= 3)
					_mm_pause();
				else
					std::this_thread::yield();

				continue;
			}

			// Now doing some kind of operation
			ThreadWorking[threadIndex].store(true);
			idleCount = 0;

		__fastloop:
			switch (p.Type)
			{
			case CULL_COLLECT:
				TraverseSceneGraph();
				break;

			case CULL_RENDER_GEOMETRY:
			{
				ProfileTimer("MOC RenderGeometry");

				IndexPair indexRawData;
				float *vertexRawData;

				GetCachedVerticesAndIndices(p.Geometry, &indexRawData, &vertexRawData);

				XMMATRIX worldProj = BSShaderUtil::GetXMFromNiPosAdjust(p.Geometry->GetWorldTransform(), MyPosAdjust);
				XMMATRIX worldViewProj = XMMatrixMultiply(worldProj, MyViewProj);

				moc->RenderTriangles(
					vertexRawData,
					indexRawData.Data,
					indexRawData.Count / 3,
					(float *)&worldViewProj,
					MaskedOcclusionCulling::BACKFACE_CW,
					MaskedOcclusionCulling::CLIP_PLANE_SIDES);

				ProfileCounterInc("MOC ObjectsRendered");
				ProfileCounterAdd("MOC TrianglesRendered", indexRawData.Count / 3);
			}
			break;

			case CULL_FLUSH:
			{
				// Merge the buffer from every other thread into this one
				while (true)
				{
					int threadsMerged = 1;

					for (uint32_t i = 0; i < 2; i++)
					{
						if (i == threadIndex)
							continue;

						if (ThreadWorking[i].load())
							continue;

						moc->MergeBuffer(MocList[i]);
						threadsMerged++;
					}

					if (threadsMerged == 2)
						break;
				}

				// Notify whoever was waiting for this
				FinalBuffer.store(moc);
			}
			break;
			}

			if (PendingPackets.try_pop(p))
				goto __fastloop;

			ThreadWorking[threadIndex].store(false);
		}

		return 0;
	}

	void ForceFlush()
	{
		CullPacket p;
		p.Geometry = nullptr;
		p.Type = CULL_FLUSH;

		FinalBuffer.store(nullptr);
		PendingPackets.push(p);

		ProfileTimer("MOC WaitForRender");
		while (!FinalBuffer.load())
		{
			_mm_pause();
			continue;
		}
	}

	void ForceClear()
	{
		for (uint32_t i = 0; i < 2; i++)
			MocList[i]->ClearBuffer();
	}

	void UpdateDepthViewTexture()
	{
		if (ui::opt::RealtimeOcclusionView)
		{
			float *pixels = new float[MOC_WIDTH * MOC_HEIGHT];
			FinalBuffer.load()->ComputePixelDepthBuffer(pixels, false);
			DepthColorize(pixels, mpGPUDepthBuf);
			BSGraphics::Renderer::GetGlobals()->m_DeviceContext->UpdateSubresource(g_OcclusionTexture, 0, nullptr, mpGPUDepthBuf, MOC_WIDTH * 4, 0);
			delete[] pixels;
		}
	}

	bool mocInit = false;

	void Init()
	{
		CreateThread(nullptr, 0, CullThread, (LPVOID)0, 0, nullptr);
		CreateThread(nullptr, 0, CullThread, (LPVOID)1, 0, nullptr);

		mocInit = true;
	}

	bool TestSphere(NiAVObject *Object)
	{
		if (!mocInit || !ui::opt::EnableOcclusionTesting || !testCam)
			return true;

		if (Object->QAppCulled())
			return true;

		if (Object->m_kWorldBound.m_fRadius <= 5.0f)
			return true;

		ProfileCounterInc("MOC CullObjectCount");
		ProfileTimer("MOC CullTest");

		//if (Object->IsTriShape() && Object->IsGeometry()->QType() != GEOMETRY_TYPE_TRISHAPE)
		//	return true;

		float sphereRadius = Object->m_kWorldBound.m_fRadius;

		XMMATRIX view = MyView;
		XMMATRIX projection = MyProj;

		// w as 1.0f
		XMVECTOR bounds = _mm_sub_ps(_mm_setr_ps(
			Object->m_kWorldBound.m_kCenter.x,
			Object->m_kWorldBound.m_kCenter.y,
			Object->m_kWorldBound.m_kCenter.z,
			1.0f),
			MyPosAdjust.AsXmm());

		// Never cull sphere if player is inside it
		if (XMVector3Length(bounds).m128_f32[0] <= sphereRadius)
			return true;

		// ------ Early depth rejection test

		XMVECTOR v = XMVectorSubtract(_mm_setzero_ps(), bounds);
		XMVECTOR closestPoint = XMVectorAdd(bounds, XMVectorScale(XMVector3Normalize(v), sphereRadius));
		closestPoint = XMVector4Transform(XMVectorSetW(closestPoint, 1.0f), MyViewProj);// Project to clip space

		float closestSpherePointW = closestPoint.m128_f32[3];

		if (closestSpherePointW < 0.000001f)
			return true;

		// ------

		XMVECTOR viewEye = { view.r[0].m128_f32[3], view.r[1].m128_f32[3], view.r[2].m128_f32[3], 0.0f };
		viewEye = XMVectorNegate(viewEye);

		XMVECTOR viewEyeSphereDirection = XMVectorSubtract(viewEye, bounds);
		float cameraSphereDistance = XMVector3Length(viewEyeSphereDirection).m128_f32[0];// distance()

		XMVECTOR viewUp = { view.r[0].m128_f32[1], view.r[1].m128_f32[1], view.r[2].m128_f32[1], 0.0f };
		XMVECTOR viewRight = XMVector3Normalize(XMVector3Cross(viewEyeSphereDirection, viewUp));

		// Help handle perspective distortion.
		// http://article.gmane.org/gmane.games.devel.algorithms/21697/
		float fRadius = cameraSphereDistance * tan(asin(sphereRadius / cameraSphereDistance));

		// Compute the offsets for the points around the sphere
		XMVECTOR vUpRadius = XMVectorScale(viewUp, fRadius);
		XMVECTOR vRightRadius = XMVectorScale(viewRight, fRadius);

		// Generate the 4 corners of the sphere in world space
		XMVECTOR vCorner0WS = XMVectorSubtract(XMVectorAdd(bounds, vUpRadius), vRightRadius);		// Top-Left
		XMVECTOR vCorner1WS = XMVectorAdd(XMVectorAdd(bounds, vUpRadius), vRightRadius);			// Top-Right
		XMVECTOR vCorner2WS = XMVectorSubtract(XMVectorSubtract(bounds, vUpRadius), vRightRadius);  // Bottom-Left
		XMVECTOR vCorner3WS = XMVectorAdd(XMVectorSubtract(bounds, vUpRadius), vRightRadius);		// Bottom-Right

		// Project the 4 corners of the sphere into clip space, then convert to normalized device coordinates
		XMVECTOR vCorner0CS = XMVector4Transform(vCorner0WS, MyViewProj);
		XMVECTOR vCorner1CS = XMVector4Transform(vCorner1WS, MyViewProj);
		XMVECTOR vCorner2CS = XMVector4Transform(vCorner2WS, MyViewProj);
		XMVECTOR vCorner3CS = XMVector4Transform(vCorner3WS, MyViewProj);

		XMVECTOR vCorner0NDC = XMVectorDivide(vCorner0CS, XMVectorSplatW(vCorner0CS));
		XMVECTOR vCorner1NDC = XMVectorDivide(vCorner1CS, XMVectorSplatW(vCorner1CS));
		XMVECTOR vCorner2NDC = XMVectorDivide(vCorner2CS, XMVectorSplatW(vCorner2CS));
		XMVECTOR vCorner3NDC = XMVectorDivide(vCorner3CS, XMVectorSplatW(vCorner3CS));

		// Bounding rect mins and maxs
		XMVECTOR xy_mins = _mm_min_ps(vCorner0NDC, _mm_min_ps(vCorner1NDC, _mm_min_ps(vCorner2NDC, vCorner3NDC)));// zw discarded
		XMVECTOR xy_maxs = _mm_max_ps(vCorner0NDC, _mm_max_ps(vCorner1NDC, _mm_max_ps(vCorner2NDC, vCorner3NDC)));// zw discarded

		auto r = FinalBuffer.load()->TestRect(xy_mins.m128_f32[0], xy_mins.m128_f32[1], xy_maxs.m128_f32[0], xy_maxs.m128_f32[1], closestSpherePointW);

		if (r != MaskedOcclusionCulling::VISIBLE)
		{
#define CONVERT_X(x) ((x) + 1.0) * 2560 * 0.5 + 0
#define CONVERT_Y(y) (1.0 - (y)) * 1440 * 0.5 + 0

			static SRWLOCK lock = SRWLOCK_INIT;

			AcquireSRWLockExclusive(&lock);

			ImGui::GetWindowDrawList()->AddRect(ImVec2(CONVERT_X(xy_mins.m128_f32[0]), CONVERT_Y(xy_mins.m128_f32[1])), ImVec2(CONVERT_X(xy_maxs.m128_f32[0]), CONVERT_Y(xy_maxs.m128_f32[1])), IM_COL32(255, 0, 0, 255));

			ReleaseSRWLockExclusive(&lock);

			return false;
		}

		ProfileCounterInc("MOC CullObjectPassed");
		return true;
	}

	bool TestBoundingSphere(BSGeometry *Geometry, bool Test, bool Draw)
	{
		if (!mocInit || !ui::opt::EnableOcclusionTesting || !testCam)
			return true;

		BSShaderProperty *shaderProperty = Geometry->QShaderProperty();

		if (!shaderProperty)
			return false;

		if (Geometry->QType() == GEOMETRY_TYPE_TRISHAPE && shaderProperty->IsExactKindOf(NiRTTI::ms_BSLightingShaderProperty) && Geometry->m_kWorldBound.m_fRadius > 10)
		{
			auto triShape = static_cast<BSTriShape *>(Geometry);
			auto rendererData = reinterpret_cast<BSGraphics::TriShape *>(triShape->QRendererData());

			XMMATRIX worldProj = BSShaderUtil::GetXMFromNiPosAdjust(triShape->GetWorldTransform(), MyPosAdjust);
			XMMATRIX worldViewProj = XMMatrixMultiply(worldProj, MyViewProj);

			XMVECTOR bounds = _mm_sub_ps(_mm_setr_ps(
				Geometry->m_kWorldBound.m_kCenter.x,
				Geometry->m_kWorldBound.m_kCenter.y,
				Geometry->m_kWorldBound.m_kCenter.z,
				0.0f),
				MyPosAdjust.AsXmm());
			/*
			// w ends up being garbage, but it doesn't matter - we ignore it anyway.
			__m128 vCenter = bounds;//Geometry->m_kWorldBound.m_kCenter.AsXmm();//_mm_loadu_ps(&mBBCenter.x);
			__m128 vHalf = _mm_load1_ps((float *)&Geometry->m_kWorldBound.m_fRadius);//_mm_loadu_ps(&mBBHalf.x);

			__m128 vMin = _mm_sub_ps(vCenter, vHalf);
			__m128 vMax = _mm_add_ps(vCenter, vHalf);

			// transforms
			__m128 xRow[2], yRow[2], zRow[2];
			xRow[0] = _mm_shuffle_ps(vMin, vMin, 0x00) * worldViewProj.r[0];
			xRow[1] = _mm_shuffle_ps(vMax, vMax, 0x00) * worldViewProj.r[0];
			yRow[0] = _mm_shuffle_ps(vMin, vMin, 0x55) * worldViewProj.r[1];
			yRow[1] = _mm_shuffle_ps(vMax, vMax, 0x55) * worldViewProj.r[1];
			zRow[0] = _mm_shuffle_ps(vMin, vMin, 0xaa) * worldViewProj.r[2];
			zRow[1] = _mm_shuffle_ps(vMax, vMax, 0xaa) * worldViewProj.r[2];

			__m128 zAllIn = _mm_castsi128_ps(_mm_set1_epi32(~0));
			__m128 screenMin = _mm_set1_ps(FLT_MAX);
			__m128 screenMax = _mm_set1_ps(-FLT_MAX);

			// Find the minimum of each component
			__m128 minvert = _mm_add_ps(worldViewProj.r[3], _mm_add_ps(_mm_add_ps(_mm_min_ps(xRow[0], xRow[1]), _mm_min_ps(yRow[0], yRow[1])), _mm_min_ps(zRow[0], zRow[1])));
			float minW = minvert.m128_f32[3];
			if (minW < 0.00000001f)
				return true;

			for (UINT i = 0; i < AABB_VERTICES; i++)
			{
				// Transform the vertex
				__m128 vert = worldViewProj.r[3];
				vert += xRow[sBBxInd[i]];
				vert += yRow[sBByInd[i]];
				vert += zRow[sBBzInd[i]];

				// We have inverted z; z is in front of near plane iff z <= w.
				__m128 vertZ = _mm_shuffle_ps(vert, vert, 0xaa); // vert.zzzz
				__m128 vertW = _mm_shuffle_ps(vert, vert, 0xff); // vert.wwww

																 // project
				__m128 xformedPos = _mm_div_ps(vert, vertW);

				// update bounds
				screenMin = _mm_min_ps(screenMin, xformedPos);
				screenMax = _mm_max_ps(screenMax, xformedPos);
			}

			MaskedOcclusionCulling::CullingResult r = moc1->TestRect(screenMin.m128_f32[0], screenMin.m128_f32[1], screenMax.m128_f32[0], screenMax.m128_f32[1], minW);
			*/
			//if (r != MaskedOcclusionCulling::VISIBLE)
			//	return false;
			{
				float sphereRadius = Geometry->m_kWorldBound.m_fRadius;

				XMMATRIX view = MyView;
				XMMATRIX projection = MyProj;

				// w as 1.0f
				XMVECTOR bounds = _mm_sub_ps(_mm_setr_ps(
					Geometry->m_kWorldBound.m_kCenter.x,
					Geometry->m_kWorldBound.m_kCenter.y,
					Geometry->m_kWorldBound.m_kCenter.z,
					1.0f),
					MyPosAdjust.AsXmm());

				// ------ Early depth rejection test

				XMVECTOR v = XMVectorSubtract(_mm_setzero_ps(), bounds);
				XMVECTOR closestPoint = XMVectorAdd(bounds, XMVectorScale(XMVector3Normalize(v), sphereRadius));
				closestPoint = XMVector4Transform(XMVectorSetW(closestPoint, 1.0f), MyViewProj);// Project to clip space

				float closestSpherePointW = closestPoint.m128_f32[3];

				if (closestSpherePointW < 0.000001f)
					return true;

				/*
				// Compute the center of the bounding sphere in screen space
				XMVECTOR Cv = XMVector4Transform(bounds, MyView);

				// compute nearest point to camera on sphere, and project it
				XMVECTOR Pv = XMVectorSubtract(Cv, XMVectorScale(XMVector3Normalize(Cv), sphereRadius));
				float closestSpherePointW = XMVector4Transform(XMVectorSetW(Pv, 1.0f), MyProj).m128_f32[3];

				if (closestSpherePointW < 0.000001f)
					return true;
					*/
				// ------

				XMVECTOR viewEye = { view.r[0].m128_f32[3], view.r[1].m128_f32[3], view.r[2].m128_f32[3], 0.0f };
				viewEye = XMVectorNegate(viewEye);

				XMVECTOR viewEyeSphereDirection = XMVectorSubtract(viewEye, bounds);
				float cameraSphereDistance = XMVector3Length(viewEyeSphereDirection).m128_f32[0];// distance()

				XMVECTOR viewUp = { view.r[0].m128_f32[1], view.r[1].m128_f32[1], view.r[2].m128_f32[1], 0.0f };
				XMVECTOR viewDirection = { view.r[0].m128_f32[2], view.r[1].m128_f32[2], view.r[2].m128_f32[2], 0.0f };
				XMVECTOR viewRight = XMVector3Normalize(XMVector3Cross(viewEyeSphereDirection, viewUp));

				// Help handle perspective distortion.
				// http://article.gmane.org/gmane.games.devel.algorithms/21697/
				float fRadius = cameraSphereDistance * tan(asin(sphereRadius / cameraSphereDistance));

				// Compute the offsets for the points around the sphere
				XMVECTOR vUpRadius = XMVectorScale(viewUp, fRadius);
				XMVECTOR vRightRadius = XMVectorScale(viewRight, fRadius);

				// Generate the 4 corners of the sphere in world space
				XMVECTOR vCorner0WS = XMVectorSubtract(XMVectorAdd(bounds, vUpRadius), vRightRadius);		// Top-Left
				XMVECTOR vCorner1WS = XMVectorAdd(XMVectorAdd(bounds, vUpRadius), vRightRadius);			// Top-Right
				XMVECTOR vCorner2WS = XMVectorSubtract(XMVectorSubtract(bounds, vUpRadius), vRightRadius);  // Bottom-Left
				XMVECTOR vCorner3WS = XMVectorAdd(XMVectorSubtract(bounds, vUpRadius), vRightRadius);		// Bottom-Right

				// Project the 4 corners of the sphere into clip space, then convert to normalized device coordinates
				XMVECTOR vCorner0CS = XMVector4Transform(vCorner0WS, MyViewProj);
				XMVECTOR vCorner1CS = XMVector4Transform(vCorner1WS, MyViewProj);
				XMVECTOR vCorner2CS = XMVector4Transform(vCorner2WS, MyViewProj);
				XMVECTOR vCorner3CS = XMVector4Transform(vCorner3WS, MyViewProj);

				XMVECTOR vCorner0NDC = XMVectorDivide(vCorner0CS, XMVectorSplatW(vCorner0CS));
				XMVECTOR vCorner1NDC = XMVectorDivide(vCorner1CS, XMVectorSplatW(vCorner1CS));
				XMVECTOR vCorner2NDC = XMVectorDivide(vCorner2CS, XMVectorSplatW(vCorner2CS));
				XMVECTOR vCorner3NDC = XMVectorDivide(vCorner3CS, XMVectorSplatW(vCorner3CS));

				// Bounding rect mins and maxs
				XMVECTOR xy_mins = _mm_min_ps(vCorner0NDC, _mm_min_ps(vCorner1NDC, _mm_min_ps(vCorner2NDC, vCorner3NDC)));// zw discarded
				XMVECTOR xy_maxs = _mm_max_ps(vCorner0NDC, _mm_max_ps(vCorner1NDC, _mm_max_ps(vCorner2NDC, vCorner3NDC)));// zw discarded

				auto r = FinalBuffer.load()->TestRect(xy_mins.m128_f32[0], xy_mins.m128_f32[1], xy_maxs.m128_f32[0], xy_maxs.m128_f32[1], closestSpherePointW);
				/*
#define CONVERT_X(x) ((x) + 1.0) * 2560 * 0.5 + 0
#define CONVERT_Y(y) (1.0 - (y)) * 1440 * 0.5 + 0

				static SRWLOCK lock = SRWLOCK_INIT;

				AcquireSRWLockExclusive(&lock);
				float d1 = Geometry->GetWorldTranslate().x - MyPosAdjust.x;
				float d2 = Geometry->GetWorldTranslate().y - MyPosAdjust.y;

				// Distance2DSqaured
				if (((d1 * d1) + (d2 * d2)) < (1000 * 1000) && ((d1 * d1) + (d2 * d2)) > (50 * 50))
				{
					ImGui::GetWindowDrawList()->AddRect(ImVec2(CONVERT_X(vCorner0NDC.m128_f32[0]), CONVERT_Y(vCorner0NDC.m128_f32[1])), ImVec2(CONVERT_X(vCorner3NDC.m128_f32[0]), CONVERT_Y(vCorner3NDC.m128_f32[1])), IM_COL32(255, 0, 0, 255));
					//ImGui::GetWindowDrawList()->AddText(ImVec2(CONVERT_X(vCorner0NDC.m128_f32[0]), CONVERT_Y(vCorner0NDC.m128_f32[1])), IM_COL32(255, 255, 255, 255), "corner0");
					//ImGui::GetWindowDrawList()->AddText(ImVec2(x, y), IM_COL32(255, 255, 255, 255), "Test Origin");


					float realX;
					float realY;
					if (testCam->WorldToScreen(Geometry->m_kWorldBound.m_kCenter, realX, realY))
					{
						ImVec2 xy = ImVec2(realX * 2560, (1 - realY) * 1440);

						NiPoint3 outBounds;
						testCam->ScreenSpaceBoundSize(Geometry->m_kWorldBound, outBounds, 1e-5);
						outBounds.x = CONVERT_X(outBounds.x);
						outBounds.y = CONVERT_Y(outBounds.y);
						
						//ImGui::GetWindowDrawList()->AddRect(ImVec2(xy.x - (outBounds.x / 2), xy.y - (outBounds.y / 2)), ImVec2(xy.x + (outBounds.x / 2), xy.y + (outBounds.y / 2)), IM_COL32(255, 255, 255, 255));

						char buf[128];
						sprintf_s(buf, "W: %g", closestSpherePointW);
						ImGui::GetWindowDrawList()->AddText(xy, IM_COL32(255, 255, 255, 255), buf);
					}
				}
				ReleaseSRWLockExclusive(&lock);
				*/
				if (r == MaskedOcclusionCulling::OCCLUDED)
					return false;
			}
		}

		return true;
	}

	struct GeometryDistEntry
	{
		BSGeometry *Geometry;
		float Distance;
	};

	std::vector<GeometryDistEntry> GeoList;

	bool RegisterGeo(BSGeometry *Geometry, bool Test, bool Draw)
	{
		if (!mocInit || !ui::opt::EnableOccluderRendering)
			return true;

		BSShaderProperty *shaderProperty = Geometry->QShaderProperty();

		if (!shaderProperty)
			return false;

		if (Geometry->QType() == GEOMETRY_TYPE_TRISHAPE && shaderProperty->IsExactKindOf(NiRTTI::ms_BSLightingShaderProperty))
		{
			auto triShape = static_cast<BSTriShape *>(Geometry);
			auto rendererData = reinterpret_cast<BSGraphics::TriShape *>(triShape->QRendererData());

			if (rendererData && rendererData->m_RawIndexData && triShape->m_TriangleCount > 1)
			{
				GeometryDistEntry entry;
				entry.Geometry = Geometry;
				entry.Distance = XMVector3Length(_mm_sub_ps(Geometry->m_kWorldBound.m_kCenter.AsXmm(), MyPosAdjust.AsXmm())).m128_f32[0];

				GeoList.push_back(entry);
			}
		}

		return true;
	}

	void SetDoHack(bool Value)
	{
		dohack = Value;
	}

	BSMultiBoundAABB *GetAABBNode(const NiAVObject *Object)
	{
		if (BSMultiBoundNode *multiBoundNode = Object->IsMultiBoundNode())
		{
			if (multiBoundNode->spMultiBound && multiBoundNode->spMultiBound->spShape)
			{
				auto shape = multiBoundNode->spMultiBound->spShape;

				if (shape->IsExactKindOf(NiRTTI::ms_BSMultiBoundAABB))
				{
					auto aabb = static_cast<BSMultiBoundAABB *>(shape);

					if (aabb->m_kHalfExtents.x > 1.0f)
						return aabb;
				}
			}
		}

		return nullptr;
	}

	bool ShouldCullLite(const NiAVObject *Object)
	{
		if (!Object)
			return true;

		if (Object->QAppCulled() && !Object->QAlwaysDraw())
			return true;

		if (!Object->IsVisualObjectI() && !Object->QAlwaysDraw())
			return true;

		return false;
	}

	void RenderRecursive(fplanes& f, const NiAVObject *Object, bool FirstLevel)
	{
		if (ShouldCullLite(Object))
			return;

		bool validBounds = Object->m_kWorldBound.m_fRadius > 1.0f;

		if (FirstLevel)
		{
			if (validBounds && Object->m_kWorldBound.m_fRadius < ui::opt::OccluderFirstLevelMinSize)
				return;
		}

		bool cull = false;

		const char *name = Object->GetName()->c_str();

		if (name && name[0] == 'L' && name[1] == '2' && name[2] == '_')
			cull = true;

		if (!cull)
		{
			DirectX::XMVECTOR center;
			
			if (auto aabbNode = GetAABBNode(Object))
			{
				center = _mm_sub_ps(aabbNode->m_kCenter.AsXmm(), BSGraphics::Renderer::GetGlobals()->m_CurrentPosAdjust.AsXmm());
				DirectX::XMVECTOR halfExtents = aabbNode->m_kHalfExtents.AsXmm();

				if (f.TestAABB(center, _mm_add_ps(halfExtents, halfExtents)))
					cull = true;
			}
			else if (Object->m_kWorldBound.m_fRadius > 10.0f)
			{
				center = _mm_sub_ps(_mm_setr_ps(
					Object->m_kWorldBound.m_kCenter.x,
					Object->m_kWorldBound.m_kCenter.y,
					Object->m_kWorldBound.m_kCenter.z,
					Object->m_kWorldBound.m_fRadius),
					BSGraphics::Renderer::GetGlobals()->m_CurrentPosAdjust.AsXmm());

				if (f.TestSphere(center))
					cull = true;
			}
		}

		if (cull)
			return;

		if (auto node = Object->IsNode())
		{
			// Don't care about leaf anim nodes (trees, bushes, plants [alpha])
			if (!node->IsExactKindOf(NiRTTI::ms_BSLeafAnimNode))
			{
				// Enumerate children, but don't render this node specifically
				for (int i = 0; i < node->GetArrayCount(); i++)
					RenderRecursive(f, node->GetAt(i), false);
			}
		}
		else
		{
			if (Object->IsGeometry() && Object->m_kWorldBound.m_fRadius > 100.0f)
			{
				float d1 = Object->GetWorldTranslate().x - BSGraphics::Renderer::GetGlobals()->m_CurrentPosAdjust.x;
				float d2 = Object->GetWorldTranslate().y - BSGraphics::Renderer::GetGlobals()->m_CurrentPosAdjust.y;

				// Distance2DSqaured
				if (((d1 * d1) + (d2 * d2)) < (ui::opt::OccluderMaxDistance * ui::opt::OccluderMaxDistance))
					RegisterGeo(Object->IsGeometry(), false, true);
			}
		}
	}

	void RenderTopLevelNode(fplanes& f, const NiNode *Node)
	{
		if (ShouldCullLite(Node))
			return;

		// Everything in this loop will be some kind of node
		for (int i = 0; i < Node->GetArrayCount(); i++)
			RenderRecursive(f, Node->GetAt(i), true);
	}

	AutoPtr(BSShaderAccumulator *, MainPassAccumulatora, 0x3257A70);
	AutoPtr(NiNode *, WorldScenegraph, 0x2F4CE30);

	void SendTraverseCommand()
	{
		CullPacket p;
		p.Geometry = nullptr;
		p.Type = CULL_COLLECT;

		PendingPackets.push(p);
	}

	void TraverseSceneGraph()
	{
		ProfileTimer("MOC TraverseSceneGraph");

		//
		// Scene graph hierarchy:
		//
		// "WorldRoot Node"            SceneGraph
		// -- "CameraRoot Node"        NiNode
		// -- "shadow scene node"      ShadowSceneNode
		// ---- "Sky"                  BSMultiBoundNode
		// ---- "Weather"              NiNode
		// ---- "LODRoot"              BSClearZNode
		// ---- "ObjectLODRoot"        NiNode
		// ------ "(null)"             BSTempNodeManager
		// ------ "(null)"             NiNode
		// ------ "Cell X"             BSMultiBoundNode
		// ------ "Cell X"             BSMultiBoundNode
		// ------ "Cell X"             BSMultiBoundNode
		// ------ "Cell X"             BSMultiBoundNode
		//
		// Cell node hierarchy:
		//
		// "Cell "Wilderness" (0, 0)" BSMultiBoundNode
		// -- "ActorNode"             NiNode
		// -- "MarkerNode"            NiNode
		// -- "LandNode"              NiNode <- TARGET
		// -- "StaticNode"            NiNode <- TARGET
		// -- "DynamicNode"           NiNode
		// -- "OcclusionPlane Node"   NiNode
		// -- "Portal Node"           NiNode
		// -- "MultiBoundNode Node"   NiNode
		// -- "Collision Node"        NiNode
		//
		GeoList.clear();
		ForceClear();

		if (!MainPassAccumulatora->m_pkCamera)
			return;

		fplanes p;
		XMMATRIX testViewProj;

		testCam = MainPassAccumulatora->m_pkCamera;
		MainPassAccumulatora->m_pkCamera->CalculateViewProjection(MyView, MyProj, testViewProj);
		p.CreateFromViewProjMatrix(testViewProj);

		MyViewProj = testViewProj;
		MyPosAdjust = BSGraphics::Renderer::GetGlobals()->m_CurrentPosAdjust;

		const NiNode *node = WorldScenegraph;	// SceneGraph
		node = node->GetAt(1)->IsNode();		// ShadowSceneNode
		node = node->GetAt(3)->IsNode();		// NiNode (ObjectLODRoot)

		// Skip the first 2 child nodes
		for (int i = 2; i < node->GetArrayCount(); i++)
		{
			if (!node->GetAt(i))
				continue;

			// Recursively render everything in the CELL
			const NiNode *cellNode = node->GetAt(i)->IsNode();

			if (!cellNode)
				continue;

			const NiNode *landNode = cellNode->GetAt(2)->IsNode();
			const NiNode *staticNode = cellNode->GetAt(3)->IsNode();

			RenderTopLevelNode(p, landNode);
			RenderTopLevelNode(p, staticNode);
		}

		// Sort front to back (approx)
		std::sort(GeoList.begin(), GeoList.end(),
		[](GeometryDistEntry& a, GeometryDistEntry& b) -> bool
		{
			return a.Distance < b.Distance;
		});

		for (GeometryDistEntry& entry : GeoList)
		{
			CullPacket p;
			p.Geometry = entry.Geometry;
			p.Type = CULL_RENDER_GEOMETRY;

			PendingPackets.push(p);
		}
	}
}