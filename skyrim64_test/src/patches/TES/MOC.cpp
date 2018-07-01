#include <DirectXMath.h>
#include "../rendering/common.h"
#include "../../common.h"
#include "BSShader/BSShaderUtil.h"
#include "BSGraphicsRenderer.h"
#include "BSBatchRenderer.h"
#include "MOC.h"

#include "NiMain/BSMultiBoundNode.h"

#include "NiMain/NiNode.h"
#include <DirectXCollision.h>
#include <smmintrin.h>
using namespace DirectX;

#include "../../../MaskedOcclusionCulling/CullingThreadpool.h"
#include "../../../MaskedOcclusionCulling/MaskedOcclusionCulling.h"

extern ID3D11Texture2D *g_OcclusionTexture;
extern ID3D11ShaderResourceView *g_OcclusionTextureSRV;

static uint8_t *mpGPUDepthBuf = (uint8_t *)_aligned_malloc(sizeof(char) * ((1920 * 1080 * 4 + 31) & 0xFFFFFFE0), 32);

namespace MOC
{
	MaskedOcclusionCulling *moc1 = nullptr;
	MaskedOcclusionCulling *moc2 = nullptr;
	CullingThreadpool *ctp1 = nullptr;
	CullingThreadpool *ctp2 = nullptr;
	CullingThreadpool *ctpCurrent = nullptr;

	void Init()
	{
		moc1 = MaskedOcclusionCulling::Create();
		moc1->SetResolution(1920, 1080);
		moc1->ClearBuffer();

		moc2 = MaskedOcclusionCulling::Create();
		moc2->SetResolution(1920, 1080);
		moc2->ClearBuffer();

		ctp1 = new CullingThreadpool(2, 2, 2);
		ctp1->SetBuffer(moc1);
		ctp1->WakeThreads();

		ctp2 = new CullingThreadpool(2, 2, 2);
		ctp2->SetBuffer(moc2);
		ctp2->WakeThreads();

		ctpCurrent = ctp1;
	}

	std::unordered_map<void *, float *> m_VertMap;
	std::unordered_map<void *, uint32_t *> m_IndexMap;

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

	void GetCachedVerticesAndIndices(BSGeometry *Geometry, uint32_t **Indices, float **Vertices)
	{
		void *dataPtr;

		const void *indexData;
		uint32_t indexCount;

		const void *vertexData;
		uint32_t vertexCount;
		uint32_t vertexStride;

		if (Geometry->QType() == GEOMETRY_TYPE_TRISHAPE)
		{
			auto triShape = static_cast<BSTriShape *>(Geometry);
			auto rendererData = reinterpret_cast<BSGraphics::TriShape *>(triShape->QRendererData());
			dataPtr = rendererData;

			if (auto itr = m_IndexMap.find(rendererData); itr != m_IndexMap.end())
				*Indices = itr->second;
			else
			{
				*Indices = nullptr;
				indexData = rendererData->m_RawIndexData;
				indexCount = triShape->m_TriangleCount * 3;
			}

			if (auto itr = m_VertMap.find(rendererData); itr != m_VertMap.end())
				*Vertices = itr->second;
			else
			{
				*Vertices = nullptr;
				vertexData = rendererData->m_RawVertexData;
				vertexCount = triShape->m_VertexCount;
				vertexStride = BSGeometry::CalculateVertexSize(rendererData->m_VertexDesc);
			}
		}
		else
		{
			__debugbreak();
		}

		// If one wasn't found, it needs conversion
		if (!*Indices)
		{
			*Indices = ConvertIndices(indexData, indexCount, vertexCount);
			m_IndexMap.insert_or_assign(dataPtr, *Indices);
		}

		if (!*Vertices)
		{
			*Vertices = ConvertVerts(vertexData, vertexCount, vertexStride);
			m_VertMap.insert_or_assign(dataPtr, *Vertices);
		}
	}

	void RemoveCachedVerticesAndIndices(void *RendererData)
	{
		auto indItr = m_IndexMap.find(RendererData);
		auto vertItr = m_VertMap.find(RendererData);

		if (indItr != m_IndexMap.end())
		{
			delete[] indItr->second;
			m_IndexMap.erase(indItr);
		}

		if (vertItr != m_VertMap.end())
		{
			delete[] vertItr->second;
			m_VertMap.erase(vertItr);
		}
	}

	void DepthColorize(const float *FloatData, uint8_t *OutColorArray)
	{
		int w = 1920;
		int h = 1080;

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

	void UpdateDepthViewTexture()
	{
		if (ui::opt::RealtimeOcclusionView)
		{
			float *pixels = new float[1920 * 1080];
			ctpCurrent->ComputePixelDepthBuffer(pixels, false);
			DepthColorize(pixels, mpGPUDepthBuf);
			BSGraphics::Renderer::GetGlobals()->m_DeviceContext->UpdateSubresource(g_OcclusionTexture, 0, nullptr, mpGPUDepthBuf, 1920 * 4, 0);
			delete[] pixels;
		}
	}

	bool dohack = false;

	bool RegisterGeo(BSGeometry *Geometry, bool Test, bool Draw)
	{
		if (Test && !dohack)
			return true;

		BSShaderProperty *shaderProperty = Geometry->QShaderProperty();

		if (!shaderProperty)
			return false;

		if (Geometry->QType() == GEOMETRY_TYPE_TRISHAPE && shaderProperty->IsExactKindOf(NiRTTI::ms_BSLightingShaderProperty))
		{
			Assert(Geometry->IsTriShape());

			auto triShape = static_cast<BSTriShape *>(Geometry);
			auto rendererData = reinterpret_cast<BSGraphics::TriShape *>(triShape->QRendererData());

			if (rendererData && rendererData->m_RawIndexData && triShape->m_TriangleCount > 1)
			{
				Assert(BSGeometry::HasVertexAttribute(rendererData->m_VertexDesc, 0));
				Assert(BSGeometry::CalculateVertexAttributeOffset(rendererData->m_VertexDesc, 0) == 0);
				Assert(BSGeometry::CalculateVertexSize(rendererData->m_VertexDesc) >= 16);

				uint32_t *indexRawData;
				float *vertexRawData;

				GetCachedVerticesAndIndices(triShape, &indexRawData, &vertexRawData);

				XMMATRIX worldProj = BSShaderUtil::GetXMFromNi(triShape->GetWorldTransform());
				XMMATRIX worldViewProj = XMMatrixMultiply(worldProj, BSGraphics::Renderer::GetGlobals()->m_ViewProjMat);

				if (Draw)
				{
					//if (ui::opt::EnableOccluderRendering && (!Geometry->QAlphaProperty() || !Geometry->QAlphaProperty()->GetAlphaTesting()))
					{
						ctpCurrent->SetMatrix((float *)&worldViewProj);

						ctpCurrent->RenderTriangles(
							vertexRawData,
							indexRawData,
							triShape->m_TriangleCount,
							MaskedOcclusionCulling::BACKFACE_CW,
							MaskedOcclusionCulling::CLIP_PLANE_SIDES);

						ProfileCounterAdd("Triangles Rendered", triShape->m_TriangleCount);
					}
				}

				MaskedOcclusionCulling::CullingResult r = MaskedOcclusionCulling::VISIBLE;

				if (Test)
				{
					if (ui::opt::EnableOcclusionTesting)
					{
						ctpCurrent->SetMatrix((float *)&worldViewProj);

						r = ctpCurrent->TestTriangles(
							vertexRawData,
							indexRawData,
							triShape->m_TriangleCount,
							MaskedOcclusionCulling::BACKFACE_CW,
							MaskedOcclusionCulling::CLIP_PLANE_SIDES);

						ProfileCounterAdd("Triangles Tested", triShape->m_TriangleCount);

						if (r != MaskedOcclusionCulling::VISIBLE)
							return false;
					}
				}
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

			//auto outVec = DirectX::XMVector3Project(DirectX::XMVectorSet(center.m128_f32[0], center.m128_f32[1], center.m128_f32[2], 0.0f), 0.0f, 0.0f, 2560.0f, 1440.0f, 0.0f, 1.0f, BSGraphics::Renderer::GetGlobals()->m_ProjMat, BSGraphics::Renderer::GetGlobals()->m_ViewMat, BSShaderUtil::GetXMFromNi(Object->GetWorldTransform()));
			/*
			if (outVec.m128_f32[2] < 1.0f)
			{
				char temp[512];
				sprintf_s(temp, "%s [%g, %g, %g]", Object->GetName()->c_str(), center.m128_f32[0], center.m128_f32[1], center.m128_f32[2]);

				ImGui::GetWindowDrawList()->AddText(ImVec2(outVec.m128_f32[0], outVec.m128_f32[1]), IM_COL32(255, 255, 255, 255), temp);
				//ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(outVec.m128_f32[0], outVec.m128_f32[1]), 100.0f, IM_COL32(255,255,255,255));
			}*/
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

	void TraverseSceneGraph()
	{
		AutoPtr(NiNode *, WorldScenegraph, 0x2F4CE30);

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
		ctpCurrent->ClearBuffer();

		fplanes p;
		p.CreateFromViewProjMatrix(BSGraphics::Renderer::GetGlobals()->m_ViewProjMat);

		const NiNode *node = WorldScenegraph;	// SceneGraph
		node = node->GetAt(1)->IsNode();		// ShadowSceneNode
		node = node->GetAt(3)->IsNode();		// NiNode (ObjectLODRoot)

		// Skip the first 2 child nodes
		for (int i = 2; i < node->GetArrayCount(); i++)
		{
			// Recursively render everything in the CELL
			const NiNode *cellNode = node->GetAt(i)->IsNode();

			if (!cellNode)
				continue;

			const NiNode *landNode = cellNode->GetAt(2)->IsNode();
			const NiNode *staticNode = cellNode->GetAt(3)->IsNode();

			RenderTopLevelNode(p, landNode);
			RenderTopLevelNode(p, staticNode);
		}

		ctpCurrent->Flush();
	}
}