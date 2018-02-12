#include "../rendering/common.h"
#include "../rendering/GpuCircularBuffer.h"
#include <d3dcompiler.h>
#include "BSGraphicsRenderer.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSVertexShader.h"
#include "BSShader/BSPixelShader.h"
#include "BSShader/BSShaderRenderTargets.h"
#include "BSReadWriteLock.h"
#include "MTRenderer.h"

namespace BSGraphics::Utility
{
	void CopyNiColorAToFloat(float *Floats, const NiColorA& Color)
	{
		Floats[0] = Color.r;
		Floats[1] = Color.g;
		Floats[2] = Color.b;
		Floats[3] = Color.a;
	}

	void CopyNiColorAToFloat(DirectX::XMVECTOR *Floats, const NiColorA& Color)
	{
		*Floats = Color.XmmVector();
	}

	void PackDynamicParticleData(uint32_t ParticleCount, class NiParticles *Particles, void *Buffer)
	{
		auto sub_140D75710 = (decltype(&PackDynamicParticleData))(g_ModuleBase + 0xD75710);
		sub_140D75710(ParticleCount, Particles, Buffer);
	}
}

namespace BSGraphics
{
	int CurrentFrameIndex;

	const uint32_t VertexIndexRingBufferSize = 32 * 1024 * 1024;
	const uint32_t ShaderConstantRingBufferSize = 32 * 1024 * 1024;
	const uint32_t RingBufferMaxFrames = 16;

	thread_local BSVertexShader *TLS_CurrentVertexShader;
	thread_local BSPixelShader *TLS_CurrentPixelShader;

	GpuCircularBuffer *DynamicBuffer;		// Holds vertices and indices
	GpuCircularBuffer *ShaderConstantBuffer;// Holds shader constant values

	ID3D11Query *FrameCompletedQueries[16];
	bool FrameCompletedQueryPending[16];

	thread_local uint64_t TestBufferUsedBits[4];
	ID3D11Buffer *TestBuffers[4][64];
	ID3D11Buffer *TestLargeBuffer;

	ID3D11Buffer *TempDynamicBuffers[11];

	const uint32_t ThresholdSize = 32;

	std::unordered_map<uint64_t, ID3D11InputLayout *> m_InputLayoutMap;
	BSReadWriteLock m_InputLayoutLock;

	Renderer *Renderer::GetGlobals()
	{
		return (Renderer *)HACK_GetThreadedGlobals();
	}

	Renderer *Renderer::GetGlobalsNonThreaded()
	{
		return (Renderer *)HACK_GetMainGlobals();
	}

	void Renderer::Initialize()
	{
		auto *renderer = GetGlobalsNonThreaded();

		for (uint32_t i = 0; i < RingBufferMaxFrames; i++)
		{
			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_EVENT;
			desc.MiscFlags = 0;

			Assert(SUCCEEDED(renderer->m_Device->CreateQuery(&desc, &FrameCompletedQueries[i])));
		}

		//
		// Various temporary dynamic buffers and dynamic ring buffer (indices + vertices)
		//
		// Temp buffers: 128 bytes to 131072 bytes 
		// Ring buffer: User set (default 32MB)
		//
		for (int i = 0; i < 11; i++)
		{
			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = (1u << (i + 7));// pow(2, i + 7)
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			Assert(SUCCEEDED(renderer->m_Device->CreateBuffer(&desc, nullptr, &TempDynamicBuffers[i])));
		}

		DynamicBuffer = new GpuCircularBuffer(renderer->m_Device, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER, VertexIndexRingBufferSize, RingBufferMaxFrames);

		//
		// Small temporary shader constant buffers and ring buffer
		//
		// Temp buffers: 0 bytes to 1008 bytes 
		// Ring buffer: User set (default 32MB)
		//
		TestBufferUsedBits[0] = 0;
		TestBufferUsedBits[1] = 0;
		TestBufferUsedBits[2] = 0;
		TestBufferUsedBits[3] = 0;

		for (int i = 0; i < 64; i++)
		{
			if (i == 0)
				continue;

			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = i * 16;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			for (int j = 0; j < 4; j++)
				Assert(SUCCEEDED(renderer->m_Device->CreateBuffer(&desc, nullptr, &TestBuffers[j][i])));
		}

		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = 4096;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		Assert(SUCCEEDED(renderer->m_Device->CreateBuffer(&desc, nullptr, &TestLargeBuffer)));

		ShaderConstantBuffer = new GpuCircularBuffer(renderer->m_Device, D3D11_BIND_CONSTANT_BUFFER, ShaderConstantRingBufferSize, RingBufferMaxFrames);

		// Make sure first-time pointers are set up too
		OnNewFrame();
	}

	void Renderer::OnNewFrame()
	{
		Assert(!FrameCompletedQueryPending[CurrentFrameIndex]);

		// Set a marker for when the GPU is done processing the previous frame
		GetGlobalsNonThreaded()->m_DeviceContext->End(FrameCompletedQueries[CurrentFrameIndex]);
		FrameCompletedQueryPending[CurrentFrameIndex] = true;

		DynamicBuffer->SwapFrame(CurrentFrameIndex);
		ShaderConstantBuffer->SwapFrame(CurrentFrameIndex);

		// "Pop" the query from 6 frames ago. This acts as a ring buffer.
		int prevQueryIndex = CurrentFrameIndex - 6;

		if (prevQueryIndex < 0)
			prevQueryIndex += 16;

		if (FrameCompletedQueryPending[prevQueryIndex])
		{
			BOOL data;
			HRESULT hr = GetGlobalsNonThreaded()->m_DeviceContext->GetData(FrameCompletedQueries[prevQueryIndex], &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH);

			// Those commands are REQUIRED to be complete by now - no exceptions
			AssertMsg(SUCCEEDED(hr) && data == TRUE, "DeviceContext::GetData() MUST SUCCEED BY NOW");

			DynamicBuffer->FreeOldFrame(prevQueryIndex);
			ShaderConstantBuffer->FreeOldFrame(prevQueryIndex);
			FrameCompletedQueryPending[prevQueryIndex] = false;
		}

		FlushThreadedVars();
		CurrentFrameIndex++;

		if (CurrentFrameIndex >= 16)
			CurrentFrameIndex = 0;
	}

	void Renderer::FlushThreadedVars()
	{
		memset(&TestBufferUsedBits, 0, sizeof(TestBufferUsedBits));

		//
		// Shaders should've been unique because each technique is different,
		// but for some reason that isn't the case.
		//
		// Other code in Skyrim might be setting the parameters before I do,
		// so it's not guaranteed to be cleared. (Pretend something is set)
		//
		TLS_CurrentVertexShader = (BSVertexShader *)0xFEFEFEFEFEFEFEFE;
		TLS_CurrentPixelShader = (BSPixelShader *)0xFEFEFEFEFEFEFEFE;
	}

	void Renderer::DrawLineShape(LineShape *GraphicsLineShape, uint32_t StartIndex, uint32_t Count)
	{
		SetVertexDescription(GraphicsLineShape->m_VertexDesc);
		SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		SyncD3DState(false);

		UINT stride = (4 * GraphicsLineShape->m_VertexDesc) & 0x3C;
		UINT offset = 0;

		m_DeviceContext->IASetVertexBuffers(0, 1, &GraphicsLineShape->m_VertexBuffer, &stride, &offset);
		m_DeviceContext->IASetIndexBuffer(GraphicsLineShape->m_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		m_DeviceContext->DrawIndexed(2 * Count, StartIndex, 0);
	}

	void Renderer::DrawTriShape(TriShape *GraphicsTriShape, uint32_t StartIndex, uint32_t Count)
	{
		SetVertexDescription(GraphicsTriShape->m_VertexDesc);
		SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		SyncD3DState(false);

		UINT stride = (4 * GraphicsTriShape->m_VertexDesc) & 0x3C;
		UINT offset = 0;

		m_DeviceContext->IASetVertexBuffers(0, 1, &GraphicsTriShape->m_VertexBuffer, &stride, &offset);
		m_DeviceContext->IASetIndexBuffer(GraphicsTriShape->m_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		m_DeviceContext->DrawIndexed(3 * Count, StartIndex, 0);
	}

	DynamicTriShape *Renderer::GetParticlesDynamicTriShape()
	{
		static DynamicTriShape particles =
		{
			m_UnknownVertexBuffer,
			m_UnknownIndexBuffer,
			0x840200004000051,
			0xFFFFFFFF,
			0,
			1,
			nullptr,
			nullptr
		};

		return &particles;
	}

	void *Renderer::MapDynamicTriShapeDynamicData(class BSDynamicTriShape *TriShape, DynamicTriShape *GraphicsTriShape, uint32_t Size)
	{
		if (Size <= 0)
			Size = GraphicsTriShape->m_VertexAllocationSize;

		return MapDynamicBuffer(Size, &GraphicsTriShape->m_VertexAllocationOffset);
	}

	void Renderer::UnmapDynamicTriShapeDynamicData(DynamicTriShape *GraphicsTriShape)
	{
		m_DeviceContext->Unmap(m_DynamicBuffers[m_CurrentDynamicBufferIndex], 0);
	}

	void Renderer::DrawDynamicTriShape(DynamicTriShape *GraphicsTriShape, uint32_t StartIndex, uint32_t Count)
	{
		DynamicTriShapeDrawData drawData;
		drawData.m_IndexBuffer = GraphicsTriShape->m_IndexBuffer;
		drawData.m_VertexBuffer = GraphicsTriShape->m_VertexBuffer;
		drawData.m_VertexDesc = GraphicsTriShape->m_VertexDesc;

		DrawDynamicTriShape(&drawData, StartIndex, Count, GraphicsTriShape->m_VertexAllocationOffset);
	}

	void Renderer::DrawDynamicTriShape(DynamicTriShapeDrawData *DrawData, uint32_t StartIndex, uint32_t Count, uint32_t VertexOffset)
	{
		SetVertexDescription(DrawData->m_VertexDesc);
		SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		SyncD3DState(false);

		ID3D11Buffer *buffers[2];
		buffers[0] = DrawData->m_VertexBuffer;
		buffers[1] = m_DynamicBuffers[m_CurrentDynamicBufferIndex];

		UINT strides[2];
		strides[0] = (4 * DrawData->m_VertexDesc) & 0x3C;
		strides[1] = (DrawData->m_VertexDesc >> 2) & 0x3C;

		UINT offsets[2];
		offsets[0] = 0;
		offsets[1] = VertexOffset;

		m_DeviceContext->IASetVertexBuffers(0, 2, buffers, strides, offsets);
		m_DeviceContext->IASetIndexBuffer(DrawData->m_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		m_DeviceContext->DrawIndexed(3 * Count, StartIndex, 0);
	}

	void Renderer::DrawParticleShaderTriShape(const void *DynamicData, uint32_t Count)
	{
		// Send dynamic data to GPU buffer
		uint32_t vertexStride = 48;
		uint32_t vertexOffset = 0;
		void *particleBuffer = MapDynamicBuffer(vertexStride * Count, &vertexOffset);

		memcpy_s(particleBuffer, vertexStride * Count, DynamicData, vertexStride * Count);
		UnmapDynamicTriShapeDynamicData(nullptr);

		// Update flags but don't update the input layout - we use a custom one here
		SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_StateUpdateFlags &= ~0x400;

		SyncD3DState(false);

		if (!m_UnknownInputLayout)
		{
			constexpr static D3D11_INPUT_ELEMENT_DESC inputDesc[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 1, DXGI_FORMAT_R8G8B8A8_SINT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			Assert(SUCCEEDED(m_Device->CreateInputLayout(
				inputDesc,
				ARRAYSIZE(inputDesc),
				m_CurrentVertexShader->m_RawBytecode,
				m_CurrentVertexShader->m_ShaderLength,
				&m_UnknownInputLayout)));
		}

		m_InputLayoutLock.LockForWrite();
		{
			uint64_t desc = m_VertexDescSetting & m_CurrentVertexShader->m_VertexDescription;
			m_InputLayoutMap.try_emplace(desc, m_UnknownInputLayout);
		}
		m_InputLayoutLock.UnlockWrite();

		m_DeviceContext->IASetInputLayout(m_UnknownInputLayout);
		m_StateUpdateFlags |= 0x400;

		m_DeviceContext->IASetIndexBuffer(m_UnknownIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		m_DeviceContext->IASetVertexBuffers(0, 1, &m_DynamicBuffers[m_CurrentDynamicBufferIndex], &vertexStride, &vertexOffset);
		m_DeviceContext->DrawIndexed(6 * (Count / 4), 0, 0);
	}

	SRWLOCK InputLayoutLock = SRWLOCK_INIT;

	void Renderer::SyncD3DState(bool Unknown)
	{
		auto renderer = BSGraphics::Renderer::GetGlobals();

		__int64 v5; // rdx
		int v10; // edx
		signed __int64 v12; // rcx
		float v14; // xmm0_4
		float v15; // xmm0_4

		renderer->UnmapDynamicConstantBuffer();

		uint64_t *v3 = (uint64_t *)renderer->qword_14304BF00;

		if (uint32_t flags = renderer->m_StateUpdateFlags; flags != 0)
		{
			if (flags & 1)
			{
				//
				// Build active render target view array
				//
				ID3D11RenderTargetView *renderTargetViews[8];
				uint32_t viewCount = 0;

				if (renderer->unknown1 == -1)
				{
					// This loops through all 8 entries ONLY IF they are not RENDER_TARGET_NONE. Otherwise break early.
					for (int i = 0; i < 8; i++)
					{
						uint32_t& rtState = renderer->m_RenderTargetStates[i];
						uint32_t rtIndex = renderer->m_RenderTargetIndexes[i];

						if (rtIndex == BSShaderRenderTargets::RENDER_TARGET_NONE)
							break;

						renderTargetViews[i] = (ID3D11RenderTargetView *)*((uint64_t *)v3 + 6 * rtIndex + 0x14B);
						viewCount++;

						if (rtState == 0)// if state == SRTM_CLEAR
						{
							renderer->m_DeviceContext->ClearRenderTargetView(renderTargetViews[i], (const FLOAT *)v3 + 2522);
							rtState = 4;// SRTM_INIT?
						}
					}
				}
				else
				{
					// Use a single RT instead. The purpose of this is unknown...
					v5 = *((uint64_t *)renderer->qword_14304BF00
						+ (signed int)renderer->unknown2
						+ 8i64 * (signed int)renderer->unknown1
						+ 1242);
					renderTargetViews[0] = (ID3D11RenderTargetView *)v5;
					viewCount = 1;

					if (!*(DWORD *)&renderer->__zz0[4])
					{
						renderer->m_DeviceContext->ClearRenderTargetView((ID3D11RenderTargetView *)v5, (float *)(char *)renderer->qword_14304BF00 + 10088);
						*(DWORD *)&renderer->__zz0[4] = 4;
					}
				}

				v10 = *(DWORD *)renderer->__zz0;
				if (v10 <= 2u || v10 == 6)
				{
					*((BYTE *)v3 + 34) = 0;
				}

				//
				// Determine which depth stencil to render to. When there's no active depth stencil
				// we simply send a nullptr to dx11.
				//
				ID3D11DepthStencilView *depthStencil = nullptr;

				if (renderer->rshadowState_iDepthStencil != -1)
				{
					v12 = renderer->rshadowState_iDepthStencilSlice
						+ 19i64 * (signed int)renderer->rshadowState_iDepthStencil;

					if (*((BYTE *)v3 + 34))
						depthStencil = (ID3D11DepthStencilView *)v3[v12 + 1022];
					else
						depthStencil = (ID3D11DepthStencilView *)v3[v12 + 1014];

					// Only clear the stencil if specific flags are set
					if (depthStencil && v10 != 3 && v10 != 4 && v10 != 5)
					{
						uint32_t clearFlags;

						switch (v10)
						{
						case 0:
						case 6:
							clearFlags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
							break;

						case 2:
							clearFlags = D3D11_CLEAR_STENCIL;
							break;

						case 1:
							clearFlags = D3D11_CLEAR_DEPTH;
							break;

						default:
							Assert(false);
							break;
						}

						renderer->m_DeviceContext->ClearDepthStencilView(depthStencil, clearFlags, 1.0f, 0);
						*(DWORD *)renderer->__zz0 = 4;
					}
				}

				renderer->m_DeviceContext->OMSetRenderTargets(viewCount, renderTargetViews, depthStencil);
			}

			// OMSetDepthStencilState
			if (flags & (0x4 | 0x8))
			{
				// OMSetDepthStencilState(m_DepthStates[m_DepthMode][m_StencilMode], m_StencilRef);
				renderer->m_DeviceContext->OMSetDepthStencilState(
					renderer->m_DepthStates[*(signed int *)&renderer->__zz0[32]][*(signed int *)&renderer->__zz0[40]],
					*(UINT *)&renderer->__zz0[44]);
			}

			// RSSetState
			if (flags & (0x1000 | 0x40 | 0x20 | 0x10))
			{
				// Cull mode, depth bias, fill mode, scissor mode, scissor rect (order unknown)
				void *wtf = renderer->m_RasterStates[0][0][0][*(signed int *)&renderer->__zz0[60]
					+ 2
					* (*(signed int *)&renderer->__zz0[56]
						+ 12
						* (*(signed int *)&renderer->__zz0[52]// Cull mode
							+ 3i64 * *(signed int *)&renderer->__zz0[48]))];

				renderer->m_DeviceContext->RSSetState((ID3D11RasterizerState *)wtf);

				flags = renderer->m_StateUpdateFlags;
				if (renderer->m_StateUpdateFlags & 0x40)
				{
					if (*(float *)&renderer->__zz0[24] != *(float *)&renderer->__zz2[640]
						|| (v14 = *(float *)&renderer->__zz0[28],
							*(float *)&renderer->__zz0[28] != *(float *)&renderer->__zz2[644]))
					{
						v14 = *(float *)&renderer->__zz2[644];
						*(DWORD *)&renderer->__zz0[24] = *(DWORD *)&renderer->__zz2[640];
						flags = renderer->m_StateUpdateFlags | 2;
						*(DWORD *)&renderer->__zz0[28] = *(DWORD *)&renderer->__zz2[644];
						renderer->m_StateUpdateFlags |= 2u;
					}
					if (*(DWORD *)&renderer->__zz0[56])
					{
						v15 = v14 - renderer->m_UnknownFloats1[0][*(signed int *)&renderer->__zz0[56]];
						flags |= 2u;
						renderer->m_StateUpdateFlags = flags;
						*(float *)&renderer->__zz0[28] = v15;
					}
				}
			}

			// RSSetViewports
			if (flags & 0x2)
			{
				renderer->m_DeviceContext->RSSetViewports(1, (D3D11_VIEWPORT *)&renderer->__zz0[8]);
			}

			// OMSetBlendState
			if (flags & 0x80)
			{
				float *blendFactor = (float *)(g_ModuleBase + 0x1E2C168);

				// Mode, write mode, alpha to coverage, blend state (order unknown)
				void *wtf = renderer->m_BlendStates[0][0][0][*(unsigned int *)&renderer->__zz2[656]
					+ 2
					* (*(signed int *)&renderer->__zz0[72]
						+ 13
						* (*(signed int *)&renderer->__zz0[68]
							+ 2i64 * *(signed int *)&renderer->__zz0[64]))];// AlphaBlendMode

				renderer->m_DeviceContext->OMSetBlendState((ID3D11BlendState *)wtf, blendFactor, 0xFFFFFFFF);
			}

			if (flags & (0x200 | 0x100))
			{
				D3D11_MAPPED_SUBRESOURCE resource;
				renderer->m_DeviceContext->Map(renderer->m_TempConstantBuffer1, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

				if (renderer->__zz0[76])
					*(float *)resource.pData = renderer->m_ScrapConstantValue;
				else
					*(float *)resource.pData = 0.0f;

				renderer->m_DeviceContext->Unmap(renderer->m_TempConstantBuffer1, 0);
			}

			// Shader input layout creation + updates
			if (!Unknown && (flags & 0x400))
			{
				m_InputLayoutLock.LockForWrite();
				{
					uint64_t desc = renderer->m_VertexDescSetting & renderer->m_CurrentVertexShader->m_VertexDescription;

					// Does the entry exist already?
					if (auto e = m_InputLayoutMap.find(desc); e != m_InputLayoutMap.end())
					{
						// It does. We're done now.
						renderer->m_DeviceContext->IASetInputLayout(e->second);
					}
					else
					{
						// Create and insert
						auto sub_140D705F0 = (__int64(__fastcall *)(unsigned __int64 a1))(g_ModuleBase + 0xD70620);
						ID3D11InputLayout *layout = (ID3D11InputLayout *)sub_140D705F0(desc);

						if (layout || desc != 0x300000000407)
							m_InputLayoutMap.emplace(desc, layout);

						renderer->m_DeviceContext->IASetInputLayout(layout);
					}
				}
				m_InputLayoutLock.UnlockWrite();
			}

			// IASetPrimitiveTopology
			if (flags & 0x800)
			{
				renderer->m_DeviceContext->IASetPrimitiveTopology(renderer->m_PrimitiveTopology);
			}

			if (Unknown)
				renderer->m_StateUpdateFlags = flags & 0x400;
			else
				renderer->m_StateUpdateFlags = 0;
		}

		SyncD3DResources();
	}

	void Renderer::SyncD3DResources()
	{
		auto *renderer = BSGraphics::Renderer::GetGlobals();

		//
		// Resource/state setting code. It's been modified to take 1 of 2 paths for each type:
		//
		// 1: modifiedBits == 0 { Do nothing }
		// 2: modifiedBits > 0  { Build minimal state change [X entries] before submitting it to DX }
		//
#define for_each_bit(itr, bits) for (unsigned long itr; _BitScanForward(&itr, bits); bits &= ~(1 << itr))

		// Compute shader unordered access views (UAVs)
		if (uint32_t bits = renderer->m_CSUAVModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFFFF00) == 0, "CSUAVModifiedBits must not exceed 7th index");

			for_each_bit(i, bits)
				renderer->m_DeviceContext->CSSetUnorderedAccessViews(i, 1, &renderer->m_CSUAVResources[i], nullptr);

			renderer->m_CSUAVModifiedBits = 0;
		}

		// Pixel shader samplers
		if (uint32_t bits = renderer->m_PSSamplerModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFF0000) == 0, "PSSamplerModifiedBits must not exceed 15th index");

			for_each_bit(i, bits)
				renderer->m_DeviceContext->PSSetSamplers(i, 1, &renderer->m_SamplerStates[renderer->m_PSSamplerAddressMode[i]][renderer->m_PSSamplerFilterMode[i]]);

			renderer->m_PSSamplerModifiedBits = 0;
		}

		// Pixel shader resources
		if (uint32_t bits = renderer->m_PSResourceModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFF0000) == 0, "PSResourceModifiedBits must not exceed 15th index");

			for_each_bit(i, bits)
			{
				// Combine PSSSR(0, 1, [rsc1]) + PSSSR(1, 1, [rsc2]) into PSSSR(0, 2, [rsc1, rsc2])
				if (bits & (1 << (i + 1)))
				{
					renderer->m_DeviceContext->PSSetShaderResources(i, 2, &renderer->m_PSResources[i]);
					bits &= ~(1 << (i + 1));
				}
				else
					renderer->m_DeviceContext->PSSetShaderResources(i, 1, &renderer->m_PSResources[i]);
			}

			renderer->m_PSResourceModifiedBits = 0;
		}

		// Compute shader samplers
		if (uint32_t bits = renderer->m_CSSamplerModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFF0000) == 0, "CSSamplerModifiedBits must not exceed 15th index");

			for_each_bit(i, bits)
				renderer->m_DeviceContext->CSSetSamplers(i, 1, &renderer->m_SamplerStates[renderer->m_CSSamplerSetting1[i]][renderer->m_CSSamplerSetting2[i]]);

			renderer->m_CSSamplerModifiedBits = 0;
		}

		// Compute shader resources
		if (uint32_t bits = renderer->m_CSResourceModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFF0000) == 0, "CSResourceModifiedBits must not exceed 15th index");

			for_each_bit(i, bits)
				renderer->m_DeviceContext->CSSetShaderResources(i, 1, &renderer->m_CSResources[i]);

			renderer->m_CSResourceModifiedBits = 0;
		}

#undef for_each_bit
	}

	void Renderer::DepthStencilStateSetDepthMode(uint32_t Mode)
	{
		if (MTRenderer::InsertCommand<MTRenderer::SetStateRenderCommand>(MTRenderer::SetStateRenderCommand::DepthStencilStateDepthMode, Mode))
			return;

		if (*(DWORD *)&__zz0[32] != Mode)
		{
			*(DWORD *)&__zz0[32] = Mode;

			// Temp var to prevent duplicate state setting? Don't know where this gets set.
			if (*(DWORD *)&__zz0[36] != Mode)
				m_StateUpdateFlags |= 0x4;
			else
				m_StateUpdateFlags &= ~0x4;
		}
	}

	void Renderer::DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef)
	{
		if (*(DWORD *)&__zz0[40] != Mode || *(DWORD *)&__zz0[44] != StencilRef)
		{
			*(DWORD *)&__zz0[40] = Mode;
			*(DWORD *)&__zz0[44] = StencilRef;
			m_StateUpdateFlags |= 0x8;
		}
	}

	void Renderer::RasterStateSetCullMode(uint32_t CullMode)
	{
		if (*(DWORD *)&__zz0[52] != CullMode)
		{
			*(DWORD *)&__zz0[52] = CullMode;
			m_StateUpdateFlags |= 0x20;
		}
	}

	void Renderer::RasterStateSetUnknown1(uint32_t Value)
	{
		if (*(DWORD *)&__zz0[56] != Value)
		{
			*(DWORD *)&__zz0[56] = Value;
			m_StateUpdateFlags |= 0x40;
		}
	}

	void Renderer::AlphaBlendStateSetMode(uint32_t Mode)
	{
		if (*(DWORD *)&__zz0[64] != Mode)
		{
			*(DWORD *)&__zz0[64] = Mode;
			m_StateUpdateFlags |= 0x80;
		}
	}

	void Renderer::AlphaBlendStateSetUnknown1(uint32_t Value)
	{
		if (*(DWORD *)&__zz0[68] != Value)
		{
			*(DWORD *)&__zz0[68] = Value;
			m_StateUpdateFlags |= 0x80;
		}
	}

	void Renderer::AlphaBlendStateSetUnknown2(uint32_t Value)
	{
		if (MTRenderer::InsertCommand<MTRenderer::SetStateRenderCommand>(MTRenderer::SetStateRenderCommand::AlphaBlendStateUnknown2, Value))
			return;

		if (*(DWORD *)&__zz0[72] != Value)
		{
			*(DWORD *)&__zz0[72] = Value;
			m_StateUpdateFlags |= 0x80;
		}
	}

	void Renderer::SetUseScrapConstantValue(bool UseStoredValue)
	{
		if (MTRenderer::InsertCommand<MTRenderer::SetStateRenderCommand>(MTRenderer::SetStateRenderCommand::UseScrapConstantValue_1, UseStoredValue))
			return;

		// When UseStoredValue is false, the constant buffer data is zeroed, but m_ScrapConstantValue is saved
		if (__zz0[76] != UseStoredValue)
		{
			__zz0[76] = UseStoredValue;
			m_StateUpdateFlags |= 0x100u;
		}
	}

	void Renderer::SetScrapConstantValue(float Value)
	{
		if (MTRenderer::InsertCommand<MTRenderer::SetStateRenderCommand>(MTRenderer::SetStateRenderCommand::UseScrapConstantValue_2, *(uint32_t *)&Value))
			return;

		if (m_ScrapConstantValue != Value)
		{
			m_ScrapConstantValue = Value;
			m_StateUpdateFlags |= 0x200u;
		}
	}

	void Renderer::SetVertexDescription(uint64_t VertexDesc)
	{
		if (m_VertexDescSetting != VertexDesc)
		{
			m_VertexDescSetting = VertexDesc;
			m_StateUpdateFlags |= 0x400;
		}
	}

	void Renderer::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology)
	{
		if (m_PrimitiveTopology != Topology)
		{
			m_PrimitiveTopology = Topology;
			m_StateUpdateFlags |= 0x800;
		}
	}

	void Renderer::SetVertexShader(BSVertexShader *Shader)
	{
		if (Shader == TLS_CurrentVertexShader)
			return;

		// The input layout (IASetInputLayout) may need to be created and updated
		TLS_CurrentVertexShader = Shader;
		m_CurrentVertexShader = Shader;
		m_StateUpdateFlags |= 0x400;
		m_DeviceContext->VSSetShader(Shader ? Shader->m_Shader : nullptr, nullptr, 0);
	}

	void Renderer::SetPixelShader(BSPixelShader *Shader)
	{
		if (Shader == TLS_CurrentPixelShader)
			return;

		TLS_CurrentPixelShader = Shader;
		m_CurrentPixelShader = Shader;
		m_DeviceContext->PSSetShader(Shader ? Shader->m_Shader : nullptr, nullptr, 0);
	}

	void Renderer::SetTexture(uint32_t Index, Texture *Resource)
	{
		SetShaderResource(Index, Resource ? Resource->m_D3DTexture : nullptr);
	}

	void Renderer::SetTextureMode(uint32_t Index, uint32_t AddressMode, uint32_t FilterMode)
	{
		SetTextureAddressMode(Index, AddressMode);
		SetTextureFilterMode(Index, FilterMode);
	}

	void Renderer::SetTextureAddressMode(uint32_t Index, uint32_t Mode)
	{
		if (m_PSSamplerAddressMode[Index] != Mode)
		{
			m_PSSamplerAddressMode[Index] = Mode;
			m_PSSamplerModifiedBits |= 1 << Index;
		}
	}

	void Renderer::SetTextureFilterMode(uint32_t Index, uint32_t Mode)
	{
		if (m_PSSamplerFilterMode[Index] != Mode)
		{
			m_PSSamplerFilterMode[Index] = Mode;
			m_PSSamplerModifiedBits |= 1 << Index;
		}
	}

	// Not a real function name. Needs to be removed.
	void Renderer::SetShaderResource(uint32_t Index, ID3D11ShaderResourceView *Resource)
	{
		if (m_PSResources[Index] != Resource)
		{
			m_PSResources[Index] = Resource;
			m_PSResourceModifiedBits |= 1 << Index;
		}
	}

	ID3D11Buffer *Renderer::MapConstantBuffer(void **DataPointer, uint32_t *AllocationSize, uint32_t *AllocationOffset, uint32_t Level)
	{
		uint32_t initialAllocSize = *AllocationSize;
		uint32_t roundedAllocSize = 0;
		ID3D11Buffer *buffer = nullptr;

		Assert(initialAllocSize > 0);

		//
		// If the user lets us, try to use the global ring buffer instead of small temporary
		// allocations. It must be used on the immediate D3D context only. No MTR here.
		//
		if (initialAllocSize > ThresholdSize && !MTRenderer::IsRenderingMultithreaded())
		{
			// Size must be rounded up to nearest 256 bytes (D3D11.1 specification)
			roundedAllocSize = (initialAllocSize + 256 - 1) & ~(256 - 1);

			*DataPointer = ShaderConstantBuffer->MapData(m_DeviceContext, roundedAllocSize, AllocationOffset, false);
			*AllocationSize = roundedAllocSize;
			buffer = ShaderConstantBuffer->D3DBuffer;
		}
		else
		{
			Assert(initialAllocSize <= 4096);

			if (Level >= ARRAYSIZE(TestBufferUsedBits))
				Level = ARRAYSIZE(TestBufferUsedBits) - 1;

			// Small constant buffer pool: round to nearest 16, determine bit index, then loop until there's a free slot
			roundedAllocSize = (initialAllocSize + 16 - 1) & ~(16 - 1);
			D3D11_MAPPED_SUBRESOURCE map;

			if (roundedAllocSize <= (63 * 16))
			{
				for (uint32_t bitIndex = roundedAllocSize / 16; bitIndex < 64;)
				{
					if ((TestBufferUsedBits[Level] & (1ull << bitIndex)) == 0)
					{
						TestBufferUsedBits[Level] |= (1ull << bitIndex);
						buffer = TestBuffers[Level][bitIndex];
						break;
					}

					// Try next largest buffer size
					bitIndex += 1;
					roundedAllocSize += 16;
				}
			}
			else
			{
				// Last-ditch effort for a large valid buffer
				roundedAllocSize = 4096;
				buffer = TestLargeBuffer;
			}

			Assert(buffer);
			Assert(SUCCEEDED(m_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)));

			*DataPointer = map.pData;
			*AllocationSize = map.RowPitch;

			if (AllocationOffset)
				*AllocationOffset = 0;
		}

		ProfileCounterAdd("CB Bytes Requested", initialAllocSize);
		ProfileCounterAdd("CB Bytes Wasted", (roundedAllocSize - initialAllocSize));
		return buffer;
	}

	void Renderer::UnmapDynamicConstantBuffer()
	{
		if (!MTRenderer::IsRenderingMultithreaded())
			ShaderConstantBuffer->UnmapData(m_DeviceContext);

		memset(&TestBufferUsedBits, 0, sizeof(TestBufferUsedBits));
	}

	void *Renderer::MapDynamicBuffer(uint32_t AllocationSize, uint32_t *AllocationOffset)
	{
		ProfileCounterAdd("VIB Bytes Requested", AllocationSize);

		//
		// Try to use the global ring buffer instead of small temporary allocations. It
		// must be used on the immediate D3D context only. No MTR here.
		//
		if (!MTRenderer::IsRenderingMultithreaded())
		{
			m_DynamicBuffers[0] = DynamicBuffer->D3DBuffer;
			m_CurrentDynamicBufferIndex = 0;
			m_FrameDataUsedSize = AllocationSize;

			return DynamicBuffer->MapData(m_DeviceContext, AllocationSize, AllocationOffset, true);
		}

		//
		// Select one of the random temporary buffers: index = ceil(log2(max(AllocationSize, 256)))
		//
		// NOTE: There might be a race condition since there's only 1 array used. If skyrim discards
		// each allocation after a draw call, this is generally OK.
		//
		DWORD logIndex = 0;
		_BitScanReverse(&logIndex, max(AllocationSize, 256));

		if ((1u << logIndex) < AllocationSize)
			logIndex += 1;

		Assert(logIndex >= 7 || logIndex < (11 + 7));

		// Adjust base index - buffers start at 256 (2^7) bytes
		ID3D11Buffer *buffer = TempDynamicBuffers[logIndex - 7];

		D3D11_MAPPED_SUBRESOURCE map;
		Assert(SUCCEEDED(m_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)));

		m_DynamicBuffers[0] = buffer;
		m_CurrentDynamicBufferIndex = 0;
		m_FrameDataUsedSize = AllocationSize;
		*AllocationOffset = 0;

		return map.pData;

		/*
		// Invalidate the others for sanity checking
		m_DynamicBuffers[1] = (ID3D11Buffer *)0x101010101;
		m_DynamicBuffers[2] = (ID3D11Buffer *)0x101010101;
		m_CommandListEndEvents[0] = (ID3D11Query *)0x101010101;
		m_CommandListEndEvents[1] = (ID3D11Query *)0x101010101;
		m_CommandListEndEvents[2] = (ID3D11Query *)0x101010101;
		*/
	}

	char tempBuffer[8192];
	void ReflectConstantBuffers(ID3D11ShaderReflection *Reflector, BSConstantGroup *Groups, uint32_t MaxGroups, std::function<const char *(int Index)> GetConstant, uint8_t *Offsets, uint32_t MaxOffsets)
	{
		D3D11_SHADER_DESC desc;
		Assert(SUCCEEDED(Reflector->GetDesc(&desc)));

		// These should always be zeroed first. TODO: Default to 0xFF like Fallout 4 does?
		memset(Offsets, 0, MaxOffsets * sizeof(uint8_t));

		if (desc.ConstantBuffers <= 0)
			return;

		int variableCount = 0;

		auto mapBufferConsts = [&](ID3D11ShaderReflectionConstantBuffer *Buffer, BSConstantGroup *Group)
		{
			Group->m_Buffer = nullptr;
			Group->m_Data = tempBuffer;

			// If this call fails, it's an invalid buffer
			D3D11_SHADER_BUFFER_DESC bufferDesc;
			if (FAILED(Buffer->GetDesc(&bufferDesc)))
				return;

			for (uint32_t i = 0; i < bufferDesc.Variables; i++)
			{
				ID3D11ShaderReflectionVariable *var = Buffer->GetVariableByIndex(i);

				D3D11_SHADER_VARIABLE_DESC varDesc;
				Assert(SUCCEEDED(var->GetDesc(&varDesc)));

				const char *ourConstName = GetConstant(variableCount);
				const char *dxConstName = varDesc.Name;

				// Ensure that variable names match with hardcoded ones in this project
				AssertMsgVa(varDesc.StartOffset % 4 == 0, "Variable '%s' is not aligned to 4", dxConstName);
				AssertMsgVa(_stricmp(ourConstName, dxConstName) == 0, "Shader constant variable name doesn't match up (%s != %s)", ourConstName, dxConstName);

				Offsets[variableCount] = varDesc.StartOffset / 4;
				variableCount++;
			}

			// Nasty type cast here, but it's how the game does it (round up to nearest 16 bytes)
			*(uintptr_t *)&Group->m_Buffer = (bufferDesc.Size + 15) & ~15;
		};

		// Each buffer is optional (nullptr if nonexistent)
		Assert(MaxGroups == 3);

		mapBufferConsts(Reflector->GetConstantBufferByName("PerGeometry"), &Groups[2]);
		mapBufferConsts(Reflector->GetConstantBufferByName("PerMaterial"), &Groups[1]);
		mapBufferConsts(Reflector->GetConstantBufferByName("PerTechnique"), &Groups[0]);
	}

	BSVertexShader *Renderer::CompileVertexShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetConstant)
	{
		// Build defines (aka convert vector->D3DCONSTANT array)
		D3D_SHADER_MACRO macros[20 + 3 + 1];
		memset(macros, 0, sizeof(macros));

		AssertMsg(Defines.size() <= 20, "Not enough space reserved for #defines and null terminator");

		for (size_t i = 0; i < Defines.size(); i++)
		{
			macros[i].Name = Defines[i].first;
			macros[i].Definition = Defines[i].second;
		}

		macros[Defines.size() + 0].Name = "VSHADER";
		macros[Defines.size() + 0].Definition = "";
		macros[Defines.size() + 1].Name = "WINPC";
		macros[Defines.size() + 1].Definition = "";
		macros[Defines.size() + 2].Name = "DX11";
		macros[Defines.size() + 2].Definition = "";

		// Compiler setup
		UINT flags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS;
		ID3DBlob *shaderBlob = nullptr;
		ID3DBlob *shaderErrors = nullptr;

		if (FAILED(D3DCompileFromFile(FilePath, macros, nullptr, "vs_main", "vs_5_0", flags, 0, &shaderBlob, &shaderErrors)))
		{
			AssertMsgVa(false, "Vertex shader compilation failed:\n\n%s", shaderErrors ? (const char *)shaderErrors->GetBufferPointer() : "Unknown error");

			if (shaderBlob)
				shaderBlob->Release();

			if (shaderErrors)
				shaderErrors->Release();

			return nullptr;
		}

		if (shaderErrors)
			shaderErrors->Release();

		void *rawPtr = malloc(sizeof(BSVertexShader) + shaderBlob->GetBufferSize());
		BSVertexShader *vs = new (rawPtr) BSVertexShader;

		// Determine constant buffer offset map and/or shader layouts
		ID3D11ShaderReflection *reflector;
		D3D11_SHADER_DESC desc;

		Assert(SUCCEEDED(D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void **)&reflector)));
		reflector->GetDesc(&desc);

		ReflectConstantBuffers(reflector, vs->m_ConstantGroups, ARRAYSIZE(vs->m_ConstantGroups), GetConstant, vs->m_ConstantOffsets, ARRAYSIZE(vs->m_ConstantOffsets));

		// Register shader with the DX runtime itself
		Assert(SUCCEEDED(m_Device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &vs->m_Shader)));

		// Final step: append raw bytecode to the end of the struct
		memcpy(vs->m_RawBytecode, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
		vs->m_ShaderLength = shaderBlob->GetBufferSize();

		reflector->Release();
		shaderBlob->Release();

		return vs;
	}

	BSPixelShader *Renderer::CompilePixelShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetSampler, std::function<const char *(int Index)> GetConstant)
	{
		// Build defines (aka convert vector->D3DCONSTANT array)
		D3D_SHADER_MACRO macros[20 + 3 + 1];
		memset(macros, 0, sizeof(macros));

		AssertMsg(Defines.size() <= 20, "Not enough space reserved for #defines and null terminator");

		for (size_t i = 0; i < Defines.size(); i++)
		{
			macros[i].Name = Defines[i].first;
			macros[i].Definition = Defines[i].second;
		}

		macros[Defines.size() + 0].Name = "PSHADER";
		macros[Defines.size() + 0].Definition = "";
		macros[Defines.size() + 1].Name = "WINPC";
		macros[Defines.size() + 1].Definition = "";
		macros[Defines.size() + 2].Name = "DX11";
		macros[Defines.size() + 2].Definition = "";

		// Compiler setup
		UINT flags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS;
		ID3DBlob *shaderBlob = nullptr;
		ID3DBlob *shaderErrors = nullptr;

		if (FAILED(D3DCompileFromFile(FilePath, macros, nullptr, "ps_main", "ps_5_0", flags, 0, &shaderBlob, &shaderErrors)))
		{
			AssertMsgVa(false, "Pixel shader compilation failed:\n\n%s", shaderErrors ? (const char *)shaderErrors->GetBufferPointer() : "Unknown error");

			if (shaderBlob)
				shaderBlob->Release();

			if (shaderErrors)
				shaderErrors->Release();

			return nullptr;
		}

		if (shaderErrors)
			shaderErrors->Release();

		void *rawPtr = malloc(sizeof(BSPixelShader));
		BSPixelShader *ps = new (rawPtr) BSPixelShader;

		// Determine constant buffer offset map and/or shader layouts
		ID3D11ShaderReflection *reflector;
		D3D11_SHADER_DESC desc;

		Assert(SUCCEEDED(D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void **)&reflector)));
		reflector->GetDesc(&desc);

		ReflectConstantBuffers(reflector, ps->m_ConstantGroups, ARRAYSIZE(ps->m_ConstantGroups), GetConstant, ps->m_ConstantOffsets, ARRAYSIZE(ps->m_ConstantOffsets));

		// Register shader with the DX runtime itself
		Assert(SUCCEEDED(m_Device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &ps->m_Shader)));

		reflector->Release();
		shaderBlob->Release();

		return ps;
	}

	CustomConstantGroup Renderer::GetShaderConstantGroup(uint32_t Size, ConstantGroupLevel Level)
	{
		CustomConstantGroup temp;

		temp.m_Buffer = MapConstantBuffer(&temp.m_Map.pData, &Size, &temp.m_UnifiedByteOffset, Level);
		temp.m_Map.DepthPitch = Size;
		temp.m_Map.RowPitch = Size;
		temp.m_Unified = (temp.m_Buffer == ShaderConstantBuffer->D3DBuffer);

		// DirectX expects you to overwrite the entire buffer. **SKYRIM DOES NOT**, so I'm zeroing it now.
		memset(temp.m_Map.pData, 0, Size);
		return temp;
	}

	ConstantGroup<BSVertexShader> Renderer::GetShaderConstantGroup(BSVertexShader *Shader, ConstantGroupLevel Level)
	{
		ConstantGroup<BSVertexShader> temp;
		temp.m_Shader = Shader;
		temp.m_Buffer = Shader->m_ConstantGroups[Level].m_Buffer;

		if (temp.m_Buffer)
		{
			D3D11_BUFFER_DESC desc;

			if ((uintptr_t)temp.m_Buffer > 0x10000)
			{
				temp.m_Buffer->GetDesc(&desc);
				Shader->m_ConstantGroups[Level].m_Buffer = (ID3D11Buffer *)desc.ByteWidth;
			}
			else
			{
				desc.ByteWidth = (uint32_t)temp.m_Buffer;
			}

			*static_cast<CustomConstantGroup *>(&temp) = GetShaderConstantGroup(desc.ByteWidth, Level);
		}
		else
		{
			temp.m_Map.pData = Shader->m_ConstantGroups[Level].m_Data;
			// Size to memset() is unknown here
		}

		return temp;
	}

	ConstantGroup<BSPixelShader> Renderer::GetShaderConstantGroup(BSPixelShader *Shader, ConstantGroupLevel Level)
	{
		ConstantGroup<BSPixelShader> temp;
		temp.m_Shader = Shader;
		temp.m_Buffer = Shader->m_ConstantGroups[Level].m_Buffer;

		if (temp.m_Buffer)
		{
			D3D11_BUFFER_DESC desc;

			if ((uintptr_t)temp.m_Buffer > 0x10000)
			{
				temp.m_Buffer->GetDesc(&desc);
				Shader->m_ConstantGroups[Level].m_Buffer = (ID3D11Buffer *)desc.ByteWidth;
			}
			else
			{
				desc.ByteWidth = (uint32_t)temp.m_Buffer;
			}

			*static_cast<CustomConstantGroup *>(&temp) = GetShaderConstantGroup(desc.ByteWidth, Level);
		}
		else
		{
			temp.m_Map.pData = Shader->m_ConstantGroups[Level].m_Data;
			// Size to memset() is unknown here
		}

		return temp;
	}

	void Renderer::FlushConstantGroup(const CustomConstantGroup *Group)
	{
		if (Group && Group->m_Buffer && !Group->m_Unified)
			m_DeviceContext->Unmap(Group->m_Buffer, 0);
	}

	void Renderer::FlushConstantGroupVSPS(const ConstantGroup<BSVertexShader> *VertexGroup, const ConstantGroup<BSPixelShader> *PixelGroup)
	{
		FlushConstantGroup(VertexGroup);
		FlushConstantGroup(PixelGroup);
	}

	void Renderer::ApplyConstantGroupVS(const CustomConstantGroup *Group, ConstantGroupLevel Level)
	{
		if (Group)
		{
			if (Group->m_Unified)
			{
				UINT offset = Group->m_UnifiedByteOffset / 16;
				UINT size = Group->m_Map.RowPitch / 16;
				((ID3D11DeviceContext1 *)m_DeviceContext)->VSSetConstantBuffers1(Level, 1, &Group->m_Buffer, &offset, &size);
			}
			else
			{
				((ID3D11DeviceContext1 *)m_DeviceContext)->VSSetConstantBuffers(Level, 1, &Group->m_Buffer);
			}
		}
	}

	void Renderer::ApplyConstantGroupPS(const CustomConstantGroup *Group, ConstantGroupLevel Level)
	{
		if (Group)
		{
			if (Group->m_Unified)
			{
				UINT offset = Group->m_UnifiedByteOffset / 16;
				UINT size = Group->m_Map.RowPitch / 16;
				((ID3D11DeviceContext1 *)m_DeviceContext)->PSSetConstantBuffers1(Level, 1, &Group->m_Buffer, &offset, &size);
			}
			else
			{
				((ID3D11DeviceContext1 *)m_DeviceContext)->PSSetConstantBuffers(Level, 1, &Group->m_Buffer);
			}
		}
	}

	void Renderer::ApplyConstantGroupVSPS(const ConstantGroup<BSVertexShader> *VertexGroup, const ConstantGroup<BSPixelShader> *PixelGroup, ConstantGroupLevel Level)
	{
		ApplyConstantGroupVS(VertexGroup, Level);
		ApplyConstantGroupPS(PixelGroup, Level);
	}
}