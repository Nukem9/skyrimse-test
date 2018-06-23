#include "../../rendering/common.h"
#include "../../../common.h"
#include "../BSGraphicsRenderer.h"
#include "../BSGraphicsRenderTargetManager.h"
#include "BSShaderRenderTargets.h"
#include "../BSBatchRenderer.h"
#include "BSShaderManager.h"
#include "BSShaderAccumulator.h"
#include "../BSReadWriteLock.h"
#include "../MTRenderer.h"

#include "../../../MaskedOcclusionCulling/CullingThreadpool.h"
#include "../../../MaskedOcclusionCulling/MaskedOcclusionCulling.h"

void BSShaderAccumulator::InitCallbackTable()
{
	// If the pointer is null, it defaults to the function at index 0
	RegisterObjectArray[0] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 00 RegisterObject_Normal
	RegisterObjectArray[1] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 01 RegisterObject_Normal
	RegisterObjectArray[2] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 02 RegisterObject_Normal
	RegisterObjectArray[3] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 03 RegisterObject_Normal
	RegisterObjectArray[4] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 04 RegisterObject_Normal
	RegisterObjectArray[5] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 05 RegisterObject_Normal
	RegisterObjectArray[6] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 06 RegisterObject_Normal
	RegisterObjectArray[7] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 07 RegisterObject_Normal
	RegisterObjectArray[8] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 08 RegisterObject_Normal
	RegisterObjectArray[9] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 09 RegisterObject_Normal
	RegisterObjectArray[10] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 10 RegisterObject_Normal
	RegisterObjectArray[11] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 11 RegisterObject_Normal
	RegisterObjectArray[12] = (RegisterObjFunc)(g_ModuleBase + 0x12E1650);	// 12 RegisterObject_ShadowMapOrMask
	RegisterObjectArray[13] = (RegisterObjFunc)(g_ModuleBase + 0x12E1650);	// 13 RegisterObject_ShadowMapOrMask
	RegisterObjectArray[14] = (RegisterObjFunc)(g_ModuleBase + 0x12E1650);	// 14 RegisterObject_ShadowMapOrMask
	RegisterObjectArray[15] = (RegisterObjFunc)(g_ModuleBase + 0x12E1650);	// 15 RegisterObject_ShadowMapOrMask
	RegisterObjectArray[16] = (RegisterObjFunc)(g_ModuleBase + 0x12E1650);	// 16 RegisterObject_ShadowMapOrMask
	RegisterObjectArray[17] = (RegisterObjFunc)(g_ModuleBase + 0x12E1650);	// 17 RegisterObject_ShadowMapOrMask
	RegisterObjectArray[18] = (RegisterObjFunc)(g_ModuleBase + 0x12E1400);	// 18 ?
	RegisterObjectArray[19] = (RegisterObjFunc)(g_ModuleBase + 0x12E1400);	// 19 ?
	RegisterObjectArray[20] = (RegisterObjFunc)(g_ModuleBase + 0x12E1340);	// 20 RegisterObject_Interface
	RegisterObjectArray[21] = (RegisterObjFunc)(g_ModuleBase + 0x12E1370);	// 21 ?
	RegisterObjectArray[22] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 22 RegisterObject_Normal
	RegisterObjectArray[23] = (RegisterObjFunc)(g_ModuleBase + 0x12E1380);	// 23 ?
	RegisterObjectArray[24] = (RegisterObjFunc)(g_ModuleBase + 0x12E15E0);	// 24 ?
	RegisterObjectArray[25] = (RegisterObjFunc)(g_ModuleBase + 0x12E1780);	// 25 RegisterObject_LOD
	RegisterObjectArray[26] = (RegisterObjFunc)(g_ModuleBase + 0x12E1780);	// 26 RegisterObject_LOD
	RegisterObjectArray[27] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 27 RegisterObject_Normal
	RegisterObjectArray[28] = (RegisterObjFunc)(g_ModuleBase + 0x12E1800);	// 28 ?
	RegisterObjectArray[29] = (RegisterObjFunc)(g_ModuleBase + 0x12E0F90);	// 29 RegisterObject_Normal

	// If the pointer is null, it defaults to the function at index 0
	FinishAccumulatingArray[0] = FinishAccumulating_Normal;						// 00 FinishAccumulating_Normal
	FinishAccumulatingArray[1] = nullptr;										// 01
	FinishAccumulatingArray[2] = nullptr;										// 02
	FinishAccumulatingArray[3] = nullptr;										// 03
	FinishAccumulatingArray[4] = nullptr;										// 04
	FinishAccumulatingArray[5] = nullptr;										// 05
	FinishAccumulatingArray[6] = nullptr;										// 06
	FinishAccumulatingArray[7] = nullptr;										// 07
	FinishAccumulatingArray[8] = nullptr;										// 08
	FinishAccumulatingArray[9] = nullptr;										// 09
	FinishAccumulatingArray[10] = nullptr;										// 10
	FinishAccumulatingArray[11] = nullptr;										// 11
	FinishAccumulatingArray[12] = FinishAccumulating_ShadowMapOrMask;			// 12 FinishAccumulating_ShadowMapOrMask
	FinishAccumulatingArray[13] = FinishAccumulating_ShadowMapOrMask;			// 13 FinishAccumulating_ShadowMapOrMask
	FinishAccumulatingArray[14] = FinishAccumulating_ShadowMapOrMask;			// 14 FinishAccumulating_ShadowMapOrMask
	FinishAccumulatingArray[15] = FinishAccumulating_ShadowMapOrMask;			// 15 FinishAccumulating_ShadowMapOrMask
	FinishAccumulatingArray[16] = FinishAccumulating_ShadowMapOrMask;			// 16 FinishAccumulating_ShadowMapOrMask
	FinishAccumulatingArray[17] = FinishAccumulating_ShadowMapOrMask;			// 17 FinishAccumulating_ShadowMapOrMask
	FinishAccumulatingArray[18] = nullptr;										// 18
	FinishAccumulatingArray[19] = nullptr;										// 19
	FinishAccumulatingArray[20] = FinishAccumulating_InterfaceElements;			// 20 FinishAccumulating_InterfaceElements
	FinishAccumulatingArray[21] = nullptr;										// 21
	FinishAccumulatingArray[22] = FinishAccumulating_FirstPerson;				// 22 FinishAccumulating_FirstPerson
	FinishAccumulatingArray[23] = (FinishAccumFunc)(g_ModuleBase + 0x12E3020);	// 23 ?
	FinishAccumulatingArray[24] = (FinishAccumFunc)(g_ModuleBase + 0x12E3030);	// 24 ?
	FinishAccumulatingArray[25] = FinishAccumulating_LODOnly;					// 25 FinishAccumulating_LODOnly
	FinishAccumulatingArray[26] = FinishAccumulating_LODOnly;					// 26 FinishAccumulating_LODOnly
	FinishAccumulatingArray[27] = nullptr;										// 27
	FinishAccumulatingArray[28] = FinishAccumulating_Unknown1;					// 28 Never used? BSSM_RENDER_PRECIPITATION_OCCLUSION_MAP?
	FinishAccumulatingArray[29] = nullptr;										// 29

	SetRenderMode(0);
}

void BSShaderAccumulator::SetRenderMode(uint32_t RenderMode)
{
	RegisterObjectCurrent = RegisterObjectArray[RenderMode];
	FinishAccumulatingCurrent = FinishAccumulatingArray[RenderMode];

	if (!RegisterObjectCurrent)
		RegisterObjectCurrent = RegisterObjectArray[0];

	if (!FinishAccumulatingCurrent)
		FinishAccumulatingCurrent = FinishAccumulatingArray[0];
}

bool BSShaderAccumulator::hk_RegisterObjectDispatch(BSGeometry *Geometry, void *Unknown)
{
	NiSkinInstance *skinInstance = Geometry->QSkinInstance();
	BSShaderProperty *shaderProperty = Geometry->QShaderProperty();

	if (!shaderProperty)
		return true;

	// BSDismemberSkinInstance::bVisible
	if (skinInstance && skinInstance->GetRTTI() == (NiRTTI *)(g_ModuleBase + 0x3039660) && !*(BYTE *)((__int64)skinInstance + 0x98))
		return true;

	if (!Geometry->QRendererData() && !skinInstance && !Geometry->IsParticlesGeom() && Geometry->QType() != GEOMETRY_TYPE_PARTICLE_SHADER_DYNAMIC_TRISHAPE)
		return true;

	bool result = RegisterObjectArray[m_RenderMode](this, Geometry, shaderProperty, Unknown);

	uint32_t v9 = *(uint32_t *)((__int64)this + 0x160);

	if (v9 != 0)
	{
		__int64 v10 = (__int64)shaderProperty->pLightData;

		if (v10)
		{
			if (v9 == 0xFFFF)
				*(uint32_t *)(v10 + 0x1C) = 0;// BSShaderPropertyLightData::uiShadowAccumFlags
			else
				*(uint32_t *)(v10 + 0x1C) |= *(uint32_t *)((__int64)this + 0x164);
		}
	}

	return result;
}

void BSShaderAccumulator::hk_FinishAccumulatingDispatch(uint32_t RenderFlags)
{
	SetRenderMode(m_RenderMode);

	if (m_RenderMode != 0)
		FinishAccumulatingCurrent(this, RenderFlags);
	else
		RenderSceneNormal(this, RenderFlags);
}

bool BSShaderAccumulator::IsGrassShadowBlacklist(uint32_t Technique)
{
	return Technique == BSSM_GRASS_SHADOW_L ||
		Technique == BSSM_GRASS_SHADOW_LS ||
		Technique == BSSM_GRASS_SHADOW_LB ||
		Technique == BSSM_GRASS_SHADOW_LSB;
}

void ApplySceneDepthShift()
{
	((void(__fastcall *)())(g_ModuleBase + 0x12F8B10))();
}

void ResetSceneDepthShift()
{
	((void(__fastcall *)())(g_ModuleBase + 0x12F8C70))();
}

void BSShaderAccumulator::FinishAccumulating_Normal(BSShaderAccumulator *Accumulator, uint32_t RenderFlags)
{
	BSGraphics::BeginEvent(L"FinishAccumulating_Normal");
	RenderSceneNormal(Accumulator, RenderFlags);
	RenderSceneNormalAlphaZ(Accumulator, RenderFlags);
	BSGraphics::EndEvent();
}

AutoPtr(BSShaderAccumulator *, ZPrePassAccumulator, 0x3257A68);
AutoPtr(BSShaderAccumulator *, MainPassAccumulator, 0x3257A70);
BSShaderAccumulator *currentAccum = nullptr;

MaskedOcclusionCulling *moc1 = nullptr;
MaskedOcclusionCulling *moc2 = nullptr;
MaskedOcclusionCulling *mocCurrent = nullptr;
CullingThreadpool *ctp = nullptr;

void Init()
{
	moc1 = MaskedOcclusionCulling::Create();
	moc1->SetResolution(1920, 1080);
	moc1->ClearBuffer();

	moc2 = MaskedOcclusionCulling::Create();
	moc2->SetResolution(1920, 1080);
	moc2->ClearBuffer();

	mocCurrent = moc1;

	//ctp = new CullingThreadpool(2, 2, 2);
	//ctp->SetBuffer(moc);
	//ctp->WakeThreads();
}

#include "BSShaderUtil.h"
bool once = false;

uint32_t *hack_convertIndices(void *input, int count, uint32_t maxVertex)
{
	uint16_t *in = (uint16_t *)input;
	uint32_t *out = new uint32_t[count];

	for (int i = 0; i < count; i++)
	{
		out[i] = in[i];

		if (out[i] >= maxVertex)
			__debugbreak();
	}

	return out;
}

static void DepthColorize(const float * inFloatArray, char	*outColorArray)
{
	int w = 1920;
	int h = 1080;

	// Find min/max w coordinate (discard cleared pixels)
	float minW = FLT_MAX, maxW = 0.0f;
	for (int i = 0; i < w*h; ++i)
	{
		if (inFloatArray[i] > 0.0f)
		{
			minW = min(minW, inFloatArray[i]);
			maxW = max(maxW, inFloatArray[i]);
		}
	}

	// Tonemap depth values
	for (int i = 0; i < w*h; ++i)
	{
		int intensity = 0;
		if (inFloatArray[i] > 0)
			intensity = (unsigned char)(223.0*(inFloatArray[i] - minW) / (maxW - minW) + 32.0);

		outColorArray[i * 4 + 0] = intensity;
		outColorArray[i * 4 + 1] = intensity;
		outColorArray[i * 4 + 2] = intensity;
		outColorArray[i * 4 + 3] = intensity;
	}
}

bool thingy;
bool dohack = false;

float *convertVerts(float *in, uint32_t count, uint32_t stride)
{
	uintptr_t data = (uintptr_t)(new float[count * 4]);
	uintptr_t base = data;
	uintptr_t uin = (uintptr_t)in;

	for (uint32_t i = 0; i < count; i++)
	{
		memcpy((void *)data, (void *)uin, 3 * sizeof(float));
		*(float *)(data + 12) = 1.0f;

		for (int j = 0; j < 4; j++)
		{
			if (std::isnan(*(float *)(data + 4 * j)) ||
				!std::isfinite(*(float *)(data + 4 * j)) ||
				*(float *)(data + 4 * j) > 50000.0f ||
				*(float *)(data + 4 * j) < -50000.0f)
				__debugbreak();
		}
		
		data += 4 * sizeof(float);
		uin += stride;
	}

	return (float *)base;
}

#include <d3dcompiler.h>

ID3DBlob *myTestVertexShaderBlob;
ID3D11VertexShader *myTestVertexShader;
ID3D11InputLayout *myTestInputLayout;

ID3DBlob *myTestPixelShaderBlob;
ID3D11PixelShader *myTestPixelShader;

void InitTest()
{
	auto renderer = BSGraphics::Renderer::GetGlobals();

	{
		static const char* vertexShader =
            "struct VS_INPUT\
            {\
            float4 pos : POSITION;\
            };\
            \
            struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
            PS_INPUT output;\
            output.pos = input.pos;\
            return output;\
            }";

		D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &myTestVertexShaderBlob, NULL);
		if (myTestVertexShaderBlob == NULL)
			Assert(false);
		if (renderer->m_Device->CreateVertexShader(myTestVertexShaderBlob->GetBufferPointer(), myTestVertexShaderBlob->GetBufferSize(), NULL, &myTestVertexShader) != S_OK)
			Assert(false);

		// Create the input layout
		D3D11_INPUT_ELEMENT_DESC local_layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,   0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		if (renderer->m_Device->CreateInputLayout(local_layout, 1, myTestVertexShaderBlob->GetBufferPointer(), myTestVertexShaderBlob->GetBufferSize(), &myTestInputLayout) != S_OK)
			Assert(false);

	}

	{
		static const char* pixelShader =
			"struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            };\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
			return (1.0f - input.pos.z) * 10.0f; \
            }";

		D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &myTestPixelShaderBlob, NULL);
		if (myTestPixelShaderBlob == NULL)
			Assert(false);
		if (renderer->m_Device->CreatePixelShader(myTestPixelShaderBlob->GetBufferPointer(), myTestPixelShaderBlob->GetBufferSize(), NULL, &myTestPixelShader) != S_OK)
			Assert(false);
	}
}

bool RegisterGeo(BSGeometry *Geometry)
{
	if (!dohack)
		return true;

	BSShaderProperty *shaderProperty = Geometry->QShaderProperty();

	if (Geometry->QType() == GEOMETRY_TYPE_TRISHAPE && *(uintptr_t *)(shaderProperty) == (g_ModuleBase + 0x18773F0))
	{
		Assert(Geometry->IsTriShape());

		auto triShape = static_cast<BSTriShape *>(Geometry);
		auto rendererData = reinterpret_cast<BSGraphics::TriShape *>(triShape->QRendererData());

		if (rendererData && rendererData->m_RawIndexData && triShape->m_TriangleCount > 1)
		{
			//
			// float X +0x0
			// float Y +0x4
			// float Z +0x8
			// float W +0xC
			//
			MaskedOcclusionCulling::VertexLayout vertexLayout(BSGeometry::CalculateVertexSize(rendererData->m_VertexDesc), 0x4, 0xC);

			Assert(BSGeometry::CalculateVertexAttributeOffset(rendererData->m_VertexDesc, 0) == 0);
			Assert(BSGeometry::HasVertexAttribute(rendererData->m_VertexDesc, 0));
			Assert(vertexLayout.mStride >= 16);

			using namespace DirectX;

			D3D11_BUFFER_DESC desc;
			rendererData->m_IndexBuffer->GetDesc(&desc);
			Assert((desc.ByteWidth / sizeof(uint16_t)) == (triShape->m_TriangleCount * 3));

			rendererData->m_VertexBuffer->GetDesc(&desc);
			uint32_t vertCount = desc.ByteWidth / vertexLayout.mStride;

			XMMATRIX World = BSShaderUtil::GetXMFromNi(triShape->GetWorldTransform());
			XMMATRIX ViewProj = BSGraphics::Renderer::GetGlobals()->m_ViewProjMat;
			DirectX::XMMATRIX worldViewProj = DirectX::XMMatrixMultiply(World, ViewProj);


			auto renderer = BSGraphics::Renderer::GetGlobals();
			UINT stride = 16;// BSGeometry::CalculateVertexSize(rendererData->m_VertexDesc);//16;// bytes

			// vertex data upload
			float *vertexRawData;
			uint32_t vertexOffset;
			{
				vertexRawData = convertVerts((float *)rendererData->m_RawVertexData, vertCount, vertexLayout.mStride);

				if (!vertexRawData)
					return true;


				mocCurrent->TransformVertices((float *)&worldViewProj, (float *)vertexRawData, vertexRawData, vertCount, MaskedOcclusionCulling::VertexLayout(16, 4, 8));

				uint32_t vertexSize = vertCount * sizeof(float) * 4;

				//void *vertdata = renderer->MapDynamicBuffer(vertexSize, &vertexOffset);
				//memcpy(vertdata, vertexRawData, vertexSize);
				//renderer->m_DeviceContext->Unmap(renderer->m_DynamicBuffers[renderer->m_CurrentDynamicBufferIndex], 0);

			}

			// index data upload
			uint32_t *indexRawData;
			uint32_t indexOffset;
			{
				indexRawData = hack_convertIndices((uint16_t *)rendererData->m_RawIndexData, triShape->m_TriangleCount * 3, vertCount);

				uint32_t indexSize = ((triShape->m_TriangleCount * 3 * sizeof(uint32_t)) + 15) & ~15;

				//void *inddata = renderer->MapDynamicBuffer(indexSize, &indexOffset);
				//memcpy(inddata, indexRawData, indexSize);
				//renderer->m_DeviceContext->Unmap(renderer->m_DynamicBuffers[renderer->m_CurrentDynamicBufferIndex], 0);
			}
			
			// draw
			/*
			renderer->m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			renderer->SetVertexDescription(rendererData->m_VertexDesc);
			renderer->SyncD3DState(false);

			renderer->m_DeviceContext->IASetInputLayout(myTestInputLayout);
			renderer->m_DeviceContext->VSSetShader(myTestVertexShader, nullptr, 0);
			renderer->m_DeviceContext->PSSetShader(myTestPixelShader, nullptr, 0);

			renderer->m_DeviceContext->IASetVertexBuffers(0, 1, &renderer->m_DynamicBuffers[renderer->m_CurrentDynamicBufferIndex], &stride, &vertexOffset);
			renderer->m_DeviceContext->IASetIndexBuffer(renderer->m_DynamicBuffers[renderer->m_CurrentDynamicBufferIndex], DXGI_FORMAT_R32_UINT, indexOffset);
			renderer->m_DeviceContext->DrawIndexed(3 * triShape->m_TriangleCount, 0, 0);
			*/

			MaskedOcclusionCulling *mocOther = moc1;
			if (mocCurrent == moc1)
				mocOther = moc2;

			//if (shaderProperty->GetFlag(BSShaderProperty::BSSP_VERTEX_ALPHA))
			if (!Geometry->QAlphaProperty() || !Geometry->QAlphaProperty()->GetAlphaTesting())
			{
				mocCurrent->RenderTriangles(
					vertexRawData,
					indexRawData,
					triShape->m_TriangleCount,
					nullptr,
					MaskedOcclusionCulling::BACKFACE_CW,
					MaskedOcclusionCulling::CLIP_PLANE_SIDES,
					MaskedOcclusionCulling::VertexLayout(16, 4, 8));
			}

			auto r = mocOther->TestTriangles(
				vertexRawData,
				indexRawData,
				triShape->m_TriangleCount,
				nullptr,
				MaskedOcclusionCulling::BACKFACE_CW,
				MaskedOcclusionCulling::CLIP_PLANE_SIDES,
				MaskedOcclusionCulling::VertexLayout(16, 4, 8));

			ProfileCounterAdd("Triangles Tested", triShape->m_TriangleCount);

			if (r == MaskedOcclusionCulling::VISIBLE)
				ProfileCounterAdd("Triangles Rendered", triShape->m_TriangleCount);

			delete[] vertexRawData;
			delete[] indexRawData;

			if (r != MaskedOcclusionCulling::VISIBLE)
				return false;

			return true;
		}
	}

	return true;
}

extern ID3D11Texture2D *g_OcclusionTexture;
extern ID3D11ShaderResourceView *g_OcclusionTextureSRV;

static char *mpGPUDepthBuf = (char*)_aligned_malloc(sizeof(char) * ((1920 * 1080 * 4 + 31) & 0xFFFFFFE0), 32);

void BSShaderAccumulator::RenderSceneNormal(BSShaderAccumulator *Accumulator, uint32_t RenderFlags)
{
	if (!once)
	{
		once = true;
		InitTest();
		Init();
	}

	if (!Accumulator->m_pkCamera)
		return;

	__int64 a1 = (__int64)Accumulator;
	auto renderer = BSGraphics::Renderer::GetGlobals();

	renderer->BeginEvent(L"BSShaderAccumulator: Draw1");

	if (*(BYTE *)(a1 + 92) && !*(BYTE*)(g_ModuleBase + 0x30528E5))
		renderer->DepthStencilStateSetDepthMode(BSGraphics::DEPTH_STENCIL_DEPTH_MODE_TESTEQUAL);

	// v7 = RenderDepthOnly()? RenderAlphaOnly()? BSShaderManager::BSS_SHADOWS?
	bool v7 = (RenderFlags & 0xA) != 0;

	if (!v7)
		ApplySceneDepthShift();

	if (Accumulator == MainPassAccumulator)
	{
		if (ui::opt::RealtimeOcclusionView)
		{
			float *pixels = new float[1920 * 1080];
			mocCurrent->ComputePixelDepthBuffer(pixels, false);
			DepthColorize(pixels, mpGPUDepthBuf);
			BSGraphics::Renderer::GetGlobals()->m_DeviceContext->UpdateSubresource(g_OcclusionTexture, 0, nullptr, mpGPUDepthBuf, 1920 * 4, 0);
			delete[] pixels;
		}

		if (mocCurrent == moc1)
			mocCurrent = moc2;
		else
			mocCurrent = moc1;

		mocCurrent->ClearBuffer();
		dohack = true;
	}

	//
	// Original draw order:
	//
	// RenderBatches
	// LowAniso
	// RenderGrass
	// RenderNoShadowGroup
	// RenderLODObjects
	// RenderLODLand
	// RenderSky
	// RenderSkyClouds
	// ???
	// BlendedDecals
	// RenderWaterStencil
	//
	{
		ProfileTimer("RenderBatches");

		// RenderBatches
		renderer->BeginEvent(L"RenderBatches");
		{
			Accumulator->RenderFromMainGroup(1, BSSM_DISTANTTREE_DEPTH, RenderFlags, -1);
		}
		renderer->EndEvent();

		// LowAniso
		renderer->BeginEvent(L"LowAniso");
		{
			Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 9);
		}
		renderer->EndEvent();

		// RenderGrass
		renderer->BeginEvent(L"RenderGrass");
		{
			Accumulator->RenderFromMainGroup(BSSM_GRASS_DIRONLY_LF, 0x5C00005C, RenderFlags, -1);
		}
		renderer->EndEvent();

		// RenderNoShadowGroup
		renderer->BeginEvent(L"RenderNoShadowGroup");
		{
			Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 8);
		}
		renderer->EndEvent();

		// RenderLODObjects
		renderer->BeginEvent(L"RenderLODObjects");
		{
			Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 1);

			if (*(BYTE *)(a1 + 92) && !*(BYTE*)(g_ModuleBase + 0x30528E5))
				renderer->DepthStencilStateSetDepthMode(BSGraphics::DEPTH_STENCIL_DEPTH_MODE_TEST_WRITE);
		}
		renderer->EndEvent();

		// RenderLODLand
		renderer->BeginEvent(L"RenderLODLand");
		{
			ProfileTimer("LOD");

			Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 0);

			if (!v7)
				ResetSceneDepthShift();
		}
		renderer->EndEvent();

		if (Accumulator == MainPassAccumulator)
			dohack = false;
	}

	// RenderSky
	renderer->BeginEvent(L"RenderSky");
	{
		renderer->SetUseAlphaTestRef(true);
		renderer->SetAlphaTestRef(128.0f / 255.0f);

		Accumulator->RenderFromMainGroup(BSSM_SKYBASEPRE, BSSM_SKY_CLOUDSFADE, RenderFlags, -1);
	}
	renderer->EndEvent();

	// RenderSkyClouds
	renderer->BeginEvent(L"RenderSkyClouds");
	{
		renderer->AlphaBlendStateSetUnknown2(11);

		Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 13);

		renderer->AlphaBlendStateSetUnknown2(1);
	}
	renderer->EndEvent();

	if (!v7)
		ApplySceneDepthShift();

	// NormalDecals?...CK doesn't have a specific name for this
	{
		renderer->AlphaBlendStateSetUnknown2(10);
		((void(__fastcall *)(BSShaderAccumulator *, unsigned int))(g_ModuleBase + 0x12E27B0))(Accumulator, RenderFlags);
	}

	// BlendedDecals
	renderer->BeginEvent(L"BlendedDecals");
	{
		renderer->AlphaBlendStateSetUnknown2(11);
		((void(__fastcall *)(BSShaderAccumulator *, unsigned int))(g_ModuleBase + 0x12E2950))(Accumulator, RenderFlags);
	}
	renderer->EndEvent();

	renderer->AlphaBlendStateSetMode(0);
	renderer->AlphaBlendStateSetUnknown2(1);

	auto sub_140D744B0 = (int(__fastcall *)())(g_ModuleBase + 0xD744E0);
	auto sub_140D69E70 = (__int64(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0xD69EA0);
	auto sub_140D69D30 = (__int64(__fastcall *)(float *a1, float a2, float a3, float a4, int a5))(g_ModuleBase + 0xD69D60);
	auto sub_1412FD120 = (signed __int64(__fastcall *)())(g_ModuleBase + 0x12FD480);
	auto sub_140D74350 = (__int64(__fastcall *)(__int64 a1, unsigned int a2, int a3, int a4, char a5))(g_ModuleBase + 0xD74380);
	auto sub_140D74370 = (void(__fastcall *)(__int64 a1, uint32_t a2, int a3, uint32_t a4))(g_ModuleBase + 0xD743A0);
	auto sub_140D69990 = (void(__fastcall *)(__int64 a1, char a2))(g_ModuleBase + 0xD699C0);
	auto sub_1412FADA0 = (__int64(__fastcall *)())(g_ModuleBase + 0x12FB100);
	auto sub_140D69DA0 = (void(__fastcall *)(DWORD *a1))(g_ModuleBase + 0xD69DD0);

	DWORD *flt_14304E490 = (DWORD *)(g_ModuleBase + 0x304E490);

	if ((RenderFlags & 0x80) != 0 && Accumulator->m_MainBatch->HasTechniquePasses(0x5C000071, 0x5C006071))
	{
		int aiSource = sub_140D744B0();

		//	bAssert(aiSource != aiTarget &&
		//		aiSource < DEPTH_STENCIL_COUNT &&
		//		aiTarget < DEPTH_STENCIL_COUNT &&
		//		aiSource != DEPTH_STENCIL_TARGET_NONE &&
		//		aiTarget != DEPTH_STENCIL_TARGET_NONE);

		renderer->m_DeviceContext->CopyResource(*(ID3D11Resource **)(g_ModuleBase + 0x3050870), *((ID3D11Resource **)flt_14304E490 + 19 * aiSource + 1015));
	}

	// RenderWaterStencil
	renderer->BeginEvent(L"RenderWaterStencil");
	{
		if (Accumulator->m_MainBatch->HasTechniquePasses(BSSM_WATER_STENCIL, BSSM_WATER_DISPLACEMENT_STENCIL_Vc))
		{
			sub_140D69E70((__int64)flt_14304E490, 2u);// BSGraphics::Renderer::ClearDepthStencil(CLEAR_DEPTH_STENCIL_TARGET_STENCIL)
			sub_140D69D30((float *)flt_14304E490, 0.0, 0.0, 0.0, 0);
			__int64 v20 = sub_1412FD120();
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 0, v20, 3, 1);// RENDER_TARGET_NORMAL_TAAMASK_SSRMASK (This can change) SRTM_NO_CLEAR
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 1u, 7, 3, 1);// RENDER_TARGET_MOTION_VECTOR SRTM_NO_CLEAR
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 2u, -1, 3, 1);// RENDER_TARGET_NONE SRTM_NO_CLEAR
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 3u, -1, 3, 1);// RENDER_TARGET_NONE SRTM_NO_CLEAR
			sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 4u, -1, 3, 1);// RENDER_TARGET_NONE SRTM_NO_CLEAR
			Accumulator->RenderFromMainGroup(BSSM_WATER_STENCIL, BSSM_WATER_DISPLACEMENT_STENCIL_Vc, RenderFlags, -1);
			sub_140D69DA0((DWORD *)flt_14304E490);
			*(DWORD *)(*(uint64_t *)((*(uint64_t*)(g_ModuleBase + 0x31F5810)) + 496) + 44i64) = 1;
		}
	}
	renderer->EndEvent();

	if (RenderFlags & 0x40)
	{
		sub_140D74370((__int64)(g_ModuleBase + 0x3051B20), 0xFFFFFFFF, 3, 0);
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 1u, -1, 3, 1);// RENDER_TARGET_NONE SRTM_NO_CLEAR
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 2u, -1, 3, 1);// RENDER_TARGET_NONE SRTM_NO_CLEAR
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 3u, -1, 3, 1);// RENDER_TARGET_NONE SRTM_NO_CLEAR
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 4u, -1, 3, 1);// RENDER_TARGET_NONE SRTM_NO_CLEAR
		sub_140D69990((__int64)flt_14304E490, 1);
		sub_1412FADA0();
		sub_140D74350((__int64)(g_ModuleBase + 0x3051B20), 0, 1, 3, 1);
		int v21 = sub_140D744B0();
		sub_140D74370((__int64)(g_ModuleBase + 0x3051B20), v21, 3, 0);

		renderer->DepthStencilStateSetDepthMode(BSGraphics::DEPTH_STENCIL_DEPTH_MODE_TEST);
	}

	if (!v7)
		ResetSceneDepthShift();

	renderer->EndEvent();
}

void BSShaderAccumulator::RenderSceneNormalAlphaZ(BSShaderAccumulator *Accumulator, uint32_t RenderFlags)
{
	((FinishAccumFunc)(g_ModuleBase + 0x12E1F70))(Accumulator, RenderFlags);
}

void BSShaderAccumulator::FinishAccumulating_ShadowMapOrMask(BSShaderAccumulator *Accumulator, uint32_t RenderFlags)
{
	if (!Accumulator->m_pkCamera)
		return;

	BSGraphics::BeginEvent(L"FinishAccumulating_ShadowMapOrMask");

	if ((RenderFlags & 0x22) == 0x20)
		ApplySceneDepthShift();

	if (RenderFlags & 0x100)
	{
		BSGraphics::Renderer::GetGlobals()->RasterStateSetCullMode(0);
		Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 15);
	}
	else
	{
		Accumulator->RenderFromMainGroup(0x2B, 0x4000002B, RenderFlags, -1);
		Accumulator->RenderFromMainGroup(BSSM_GRASS_DIRONLY_LF, 0x5C00005C, RenderFlags, -1);

		Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 1);
		Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 9);

		((void(__fastcall *)(BSShaderAccumulator *, uint32_t))(g_ModuleBase + 0x12E2950))(Accumulator, RenderFlags);
	}

	if ((RenderFlags & 0x22) == 0x20)
		ResetSceneDepthShift();

	BSGraphics::EndEvent();
}

void BSShaderAccumulator::FinishAccumulating_InterfaceElements(BSShaderAccumulator *Accumulator, uint32_t RenderFlags)
{
	BSGraphics::BeginEvent(L"FinishAccumulating_InterfaceElements");
	((FinishAccumFunc)(g_ModuleBase + 0x12E2FE0))(Accumulator, RenderFlags);
	BSGraphics::EndEvent();
}

void BSShaderAccumulator::FinishAccumulating_FirstPerson(BSShaderAccumulator *Accumulator, uint32_t RenderFlags)
{
	BSGraphics::BeginEvent(L"FinishAccumulating_FirstPerson");
	((FinishAccumFunc)(g_ModuleBase + 0x12E2B20))(Accumulator, RenderFlags);
	BSGraphics::EndEvent();
}

void BSShaderAccumulator::FinishAccumulating_LODOnly(BSShaderAccumulator *Accumulator, uint32_t RenderFlags)
{
	if (!Accumulator->m_pkCamera)
		return;

	BSGraphics::BeginEvent(L"FinishAccumulating_LODOnly");

	BSShaderManager::bLODLandscapeNoise = false;
	Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 0);
	Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 1);
	BSShaderManager::bLODLandscapeNoise = true;

	BSGraphics::EndEvent();
}

void BSShaderAccumulator::FinishAccumulating_Unknown1(BSShaderAccumulator *Accumulator, uint32_t RenderFlags)
{
	if (!Accumulator->m_pkCamera)
		return;

	BSGraphics::BeginEvent(L"FinishAccumulating_Unknown1");

	Accumulator->RenderFromMainGroup(1, BSSM_BLOOD_SPLATTER, RenderFlags, 14);

	BSGraphics::EndEvent();
}

void BSShaderAccumulator::RenderFromMainGroup(uint32_t StartTechnique, uint32_t EndTechnique, uint32_t RenderFlags, int GroupType)
{
	if (GroupType == -1)
	{
		RenderTechniques(StartTechnique, EndTechnique, RenderFlags, -1);
	}
	else
	{
		auto pass = m_MainBatch->m_Groups[GroupType];

		if (pass)
		{
			if (pass->UnkByte1 & 1)
				pass->Render(RenderFlags);
			else
				RenderTechniques(StartTechnique, EndTechnique, RenderFlags, GroupType);
		}
	}
}

void BSShaderAccumulator::RenderTechniques(uint32_t StartTechnique, uint32_t EndTechnique, uint32_t RenderFlags, int GroupType)
{
	Assert(GroupType >= -1 && GroupType < 16);
	Assert(StartTechnique <= EndTechnique);

	BSBatchRenderer::RenderGroup *group = nullptr;
	BSBatchRenderer *batch = nullptr;

	// Always run the full function because I'm not sure if the structure
	// is used somewhere important.
	MTRenderer::InsertCommand<MTRenderer::SetAccumulatorRenderCommand>(this);
	BSShaderManager::SetCurrentAccumulator(this);

	if (GroupType <= -1)
	{
		// Wildcard: render everything with the given techniques
		group = nullptr;
		batch = this->m_MainBatch;
	}
	else
	{
		// Render a single group with given techniques
		group = this->m_MainBatch->m_Groups[GroupType];
		batch = group->m_BatchRenderer;
	}

	m_CurrentTech = 0;

	if (batch)
	{
		auto *currentNode = &batch->m_UnknownList;

		batch->m_StartingTech = StartTechnique;
		batch->m_EndingTech = EndTechnique;

		m_CurrentGroupIndex = 0;
		m_HasPendingDraws = batch->sub_14131E700(m_CurrentTech, m_CurrentGroupIndex, (__int64)&currentNode);

		while (m_HasPendingDraws)
		{
			if (IsGrassShadowBlacklist(m_CurrentTech) && (*(BYTE *)((__int64)this + 296) || *(BYTE *)((__int64)this + 297)))// if (is grass shadow) ???
				m_HasPendingDraws = batch->sub_14131ECE0(m_CurrentTech, m_CurrentGroupIndex, (__int64)&currentNode);// Probably discards pass, returns true if there's remaining sub passes
			else
				m_HasPendingDraws = batch->sub_14131E960(m_CurrentTech, m_CurrentGroupIndex, (__int64)&currentNode, RenderFlags);
		}
	}
	else
	{
		// No batcher available to render these, so just drop them
		m_HasPendingDraws = false;
	}

	if (group)
		group->Unregister();

	MTRenderer::ClearShaderAndTechnique();
}