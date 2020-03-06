#include "../../../common.h"
#include <mutex>
#include "../../rendering/GpuCircularBuffer.h"
#include "../NiMain/BSGeometry.h"
#include "BSGraphicsRenderer.h"
#include "BSGraphicsRenderTargetManager.h"

#define CHECK_RESULT(ReturnVar, Statement) do { (ReturnVar) = (Statement); AssertMsgVa(SUCCEEDED(ReturnVar), "Renderer target '%s' creation failed. HR = 0x%X.", Name, (ReturnVar)); } while (0)

thread_local D3D_PRIMITIVE_TOPOLOGY TopoOverride;

namespace BSGraphics
{
	std::mutex InputLayoutLock;
	std::unordered_map<uint64_t, ID3D11InputLayout *> InputLayoutMap;
	std::unordered_map<void *, std::pair<std::unique_ptr<uint8_t[]>, size_t>> ShaderBytecodeMap;

	const uint32_t ShaderConstantRingBufferSize = 32 * 1024 * 1024;
	const uint32_t RingBufferMaxFrames = 4;
	uint32_t CurrentFrameIndex = 0;

	ID3D11Query *FrameCompletedQueries[RingBufferMaxFrames];
	bool FrameCompletedQueryPending[RingBufferMaxFrames];

	GpuCircularBuffer *ShaderConstantBuffer;

	void BeginEvent(wchar_t *Name)
	{
	}

	void EndEvent()
	{
	}

	Renderer *Renderer::QInstance()
	{
		return (Renderer *)(g_ModuleBase + 0x304E490);
	}

	void Renderer::Initialize()
	{
		for (uint32_t i = 0; i < RingBufferMaxFrames; i++)
		{
			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_EVENT;
			desc.MiscFlags = 0;

			Assert(SUCCEEDED(Data.pDevice->CreateQuery(&desc, &FrameCompletedQueries[i])));
		}

		ShaderConstantBuffer = new GpuCircularBuffer(Data.pDevice, D3D11_BIND_CONSTANT_BUFFER, ShaderConstantRingBufferSize, RingBufferMaxFrames);
	}

	void Renderer::OnNewFrame()
	{
		Assert(!FrameCompletedQueryPending[CurrentFrameIndex]);

		// Set a marker for when the GPU is done processing the previous frame
		Data.pContext->End(FrameCompletedQueries[CurrentFrameIndex]);
		FrameCompletedQueryPending[CurrentFrameIndex] = true;

		ShaderConstantBuffer->SwapFrame(CurrentFrameIndex);

		// "Pop" the query from the oldest rendered frame
		int prevQueryIndex = CurrentFrameIndex - (RingBufferMaxFrames - 1);

		if (prevQueryIndex < 0)
			prevQueryIndex += RingBufferMaxFrames;

		if (FrameCompletedQueryPending[prevQueryIndex])
		{
			BOOL data;
			HRESULT hr = Data.pContext->GetData(FrameCompletedQueries[prevQueryIndex], &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH);

			// Those commands are REQUIRED to be complete by now - no exceptions
			AssertMsg(SUCCEEDED(hr) && data == TRUE, "DeviceContext::GetData() MUST SUCCEED BY NOW");

			ShaderConstantBuffer->FreeOldFrame(prevQueryIndex);
			FrameCompletedQueryPending[prevQueryIndex] = false;
		}

		CurrentFrameIndex++;

		if (CurrentFrameIndex >= RingBufferMaxFrames)
			CurrentFrameIndex = 0;
	}

	void Renderer::Lock()
	{
		EnterCriticalSection(&Data.RendererLock);
	}

	void Renderer::Unlock()
	{
		LeaveCriticalSection(&Data.RendererLock);
	}

	void Renderer::BeginEvent(wchar_t *Marker) const
	{
		Data.pContext->BeginEventInt(Marker, 0);
	}

	void Renderer::EndEvent() const
	{
		Data.pContext->EndEvent();
	}

	void Renderer::SetResourceName(ID3D11DeviceChild *Resource, const char *Format, ...)
	{
		if (!Resource)
			return;

		char buffer[1024];
		va_list va;

		va_start(va, Format);
		int len = _vsnprintf_s(buffer, _TRUNCATE, Format, va);
		va_end(va);

		Resource->SetPrivateData(WKPDID_D3DDebugObjectName, len, buffer);
	}

	void Renderer::CreateRenderTarget(uint32_t TargetIndex, const char *Name, const RenderTargetProperties *Properties)
	{
		Assert(TargetIndex < RENDER_TARGET_COUNT && TargetIndex != RENDER_TARGET_NONE);

		auto device = Data.pDevice;
		auto target = &Data.pRenderTargets[TargetIndex];

		HRESULT hr = S_OK;

		if (Properties->iMipLevel == -1)
		{
			D3D11_TEXTURE2D_DESC texDesc;
			texDesc.Width = Properties->uiWidth;
			texDesc.Height = Properties->uiHeight;
			texDesc.MipLevels = (Properties->bAllowMipGeneration == false);
			texDesc.ArraySize = 1;
			texDesc.Format = Properties->eFormat;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;

			if (Properties->bSupportUnorderedAccess)
				texDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

			if (Properties->bAllowMipGeneration)
				texDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

			CHECK_RESULT(hr, device->CreateTexture2D(&texDesc, nullptr, &target->Texture));
			CHECK_RESULT(hr, device->CreateRenderTargetView(target->Texture, nullptr, &target->RTV));
			CHECK_RESULT(hr, device->CreateShaderResourceView(target->Texture, nullptr, &target->SRV));

			SetResourceName(target->Texture, "%s TEX2D", Name);
			SetResourceName(target->RTV, "%s RTV", Name);
			SetResourceName(target->SRV, "%s SRV", Name);

			if (Properties->bCopyable)
			{
				texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

				CHECK_RESULT(hr, device->CreateTexture2D(&texDesc, nullptr, &target->TextureCopy));
				CHECK_RESULT(hr, device->CreateShaderResourceView(target->TextureCopy, nullptr, &target->SRVCopy));

				SetResourceName(target->TextureCopy, "%s COPY TEX2D", Name);
				SetResourceName(target->SRVCopy, "%s COPY SRV", Name);
			}

			if (Properties->bSupportUnorderedAccess)
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
				uavDesc.Format = Properties->eFormat;
				uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = 0;

				CHECK_RESULT(hr, device->CreateUnorderedAccessView(target->Texture, &uavDesc, &target->UAV));

				SetResourceName(target->UAV, "%s UAV", Name);
			}
		}
		else
		{
			ID3D11Texture2D *textureTarget = Data.pRenderTargets[Properties->uiTextureTarget].Texture;

			AssertMsg(textureTarget, "Can't create a render texture on a specified mip level because the texture has not been created.");

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
			rtvDesc.Format = Properties->eFormat;
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;

			CHECK_RESULT(hr, device->CreateRenderTargetView(textureTarget, &rtvDesc, &target->RTV));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = Properties->eFormat;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = Properties->iMipLevel;
			srvDesc.Texture2D.MipLevels = 1;

			CHECK_RESULT(hr, device->CreateShaderResourceView(textureTarget, &srvDesc, &target->SRV));

			if (Properties->bSupportUnorderedAccess)
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
				uavDesc.Format = Properties->eFormat;
				uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = Properties->iMipLevel;

				CHECK_RESULT(hr, device->CreateUnorderedAccessView(textureTarget, &uavDesc, &target->UAV));
			}

			SetResourceName(target->RTV, "%s MIP%d RTV", Name, Properties->iMipLevel);
			SetResourceName(target->SRV, "%s MIP%d SRV", Name, Properties->iMipLevel);
			SetResourceName(target->UAV, "%s MIP%d UAV", Name, Properties->iMipLevel);
		}
	}

	void Renderer::CreateDepthStencil(uint32_t TargetIndex, const char *Name, const DepthStencilTargetProperties *Properties)
	{
		Assert(TargetIndex < RENDER_TARGET_COUNT && TargetIndex != RENDER_TARGET_NONE);

		auto device = Data.pDevice;
		auto target = &Data.pDepthStencils[TargetIndex];

		HRESULT hr = S_OK;

		// Create backing texture
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = Properties->uiWidth;
		texDesc.Height = Properties->uiHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = Properties->uiArraySize;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		if (Properties->Use16BitsDepth)
			texDesc.Format = DXGI_FORMAT_R16_TYPELESS;
		else
			texDesc.Format = DXGI_FORMAT_R32_TYPELESS;

		if (Properties->Stencil)
			texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

		CHECK_RESULT(hr, device->CreateTexture2D(&texDesc, nullptr, &target->Texture));
		SetResourceName(target->Texture, "%s TEX2D", Name);

		// Depth stencil 1 (read / write / main)
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc1;
		dsvDesc1.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc1.Flags = 0;
		dsvDesc1.Texture2D.MipSlice = 0;

		if (Properties->Use16BitsDepth)
			dsvDesc1.Format = DXGI_FORMAT_D16_UNORM;
		else
			dsvDesc1.Format = DXGI_FORMAT_D32_FLOAT;

		if (Properties->Stencil)
			dsvDesc1.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// Depth stencil 2 (read only / copy)
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc2;
		dsvDesc2.Format = dsvDesc1.Format;
		dsvDesc2.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc2.Flags = D3D11_DSV_READ_ONLY_DEPTH;
		dsvDesc2.Texture2D.MipSlice = 0;

		if (Properties->Stencil)
			dsvDesc2.Flags |= D3D11_DSV_READ_ONLY_STENCIL;

		for (uint32_t i = 0; i < Properties->uiArraySize; i++)
		{
			if (Properties->uiArraySize > 1)
			{
				dsvDesc1.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc1.Texture2DArray.MipSlice = 0;
				dsvDesc1.Texture2DArray.ArraySize = 1;
				dsvDesc1.Texture2DArray.FirstArraySlice = i;

				dsvDesc2.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc2.Texture2DArray.MipSlice = 0;
				dsvDesc2.Texture2DArray.ArraySize = 1;
				dsvDesc2.Texture2DArray.FirstArraySlice = i;
			}

			CHECK_RESULT(hr, device->CreateDepthStencilView(target->Texture, &dsvDesc1, &target->Views[i]));
			CHECK_RESULT(hr, device->CreateDepthStencilView(target->Texture, &dsvDesc2, &target->ReadOnlyViews[i]));

			SetResourceName(target->Views[i], "%s DSV PRI SLICE%u", Name, i);
			SetResourceName(target->ReadOnlyViews[i], "%s DSV SEC SLICE%u", Name, i);
		}

		// Shader resource access
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		if (Properties->Use16BitsDepth)
			srvDesc.Format = DXGI_FORMAT_R16_UNORM;
		else
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

		if (Properties->Stencil)
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		if (Properties->uiArraySize > 1)
		{
			srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = Properties->uiArraySize;
		}

		CHECK_RESULT(hr, device->CreateShaderResourceView(target->Texture, &srvDesc, &target->DepthSRV));

		if (Properties->Stencil)
		{
			srvDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
			CHECK_RESULT(hr, device->CreateShaderResourceView(target->Texture, &srvDesc, &target->StencilSRV));
		}

		SetResourceName(target->DepthSRV, "%s DEPTH SRV", Name);
		SetResourceName(target->StencilSRV, "%s STENCIL SRV", Name);
	}

	void Renderer::CreateCubemapRenderTarget(uint32_t TargetIndex, const char *Name, const CubeMapRenderTargetProperties *Properties)
	{
		Assert(TargetIndex < RENDER_TARGET_CUBEMAP_COUNT);

		auto device = Data.pDevice;
		auto data = &Data.pCubemapRenderTargets[TargetIndex];

		HRESULT hr = S_OK;

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = Properties->uiWidth;
		texDesc.Height = Properties->uiHeight;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 6;
		texDesc.Format = Properties->eFormat;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		CHECK_RESULT(hr, device->CreateTexture2D(&texDesc, nullptr, &data->Texture));
		SetResourceName(data->Texture, "%s TEX2D", Name);

		// Create a separate render target for each side
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = texDesc.Format;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.ArraySize = 1;

		for (uint32_t i = 0; i < 6; i++)
		{
			rtvDesc.Texture2DArray.FirstArraySlice = i;

			CHECK_RESULT(hr, device->CreateRenderTargetView(data->Texture, &rtvDesc, &data->CubeSideRTV[i]));
			SetResourceName(data->Texture, "%s SIDE %u RTV", Name, i);
		}

		// Shader sampler
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = 1;

		CHECK_RESULT(hr, device->CreateShaderResourceView(data->Texture, &srvDesc, &data->SRV));
		SetResourceName(data->Texture, "%s SRV", Name);
	}

	void Renderer::DestroyRenderTarget(uint32_t TargetIndex)
	{
		auto data = &Data.pRenderTargets[TargetIndex];

		if (data->Texture)
			data->Texture->Release();

		if (data->TextureCopy)
			data->TextureCopy->Release();

		if (data->RTV)
			data->RTV->Release();

		if (data->SRV)
			data->SRV->Release();

		if (data->SRVCopy)
			data->SRVCopy->Release();

		if (data->UAV)
			data->UAV->Release();

		memset(data, 0, sizeof(RenderTargetData));
	}

	void Renderer::DestroyDepthStencil(uint32_t TargetIndex)
	{
		auto data = &Data.pDepthStencils[TargetIndex];

		if (data->Texture)
			data->Texture->Release();

		for (ID3D11DepthStencilView *view : data->Views)
		{
			if (view)
				view->Release();
		}

		for (ID3D11DepthStencilView *view : data->ReadOnlyViews)
		{
			if (view)
				view->Release();
		}

		if (data->DepthSRV)
			data->DepthSRV->Release();

		if (data->StencilSRV)
			data->StencilSRV->Release();

		memset(data, 0, sizeof(DepthStencilData));
	}

	void Renderer::DestroyCubemapRenderTarget(uint32_t TargetIndex)
	{
		auto data = &Data.pCubemapRenderTargets[TargetIndex];

		if (data->Texture)
			data->Texture->Release();

		for (ID3D11RenderTargetView *view : data->CubeSideRTV)
		{
			if (view)
				view->Release();
		}

		if (data->SRV)
			data->SRV->Release();

		memset(data, 0, sizeof(CubemapRenderTargetData));
	}

	void Renderer::DrawLineShape(LineShape *GraphicsLineShape, uint32_t StartIndex, uint32_t Count)
	{
		SetVertexDescription(GraphicsLineShape->m_VertexDesc);
		SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		SetDirtyStates(false);

		uint32_t stride = BSGeometry::CalculateVertexSize(GraphicsLineShape->m_VertexDesc);
		uint32_t offset = 0;

		Data.pContext->IASetVertexBuffers(0, 1, &GraphicsLineShape->m_VertexBuffer, &stride, &offset);
		Data.pContext->IASetIndexBuffer(GraphicsLineShape->m_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		Data.pContext->DrawIndexed(Count * 2, StartIndex, 0);
	}

	void Renderer::DrawTriShape(TriShape *GraphicsTriShape, uint32_t StartIndex, uint32_t Count)
	{
		SetVertexDescription(GraphicsTriShape->m_VertexDesc);
		SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		SetDirtyStates(false);

		uint32_t stride = BSGeometry::CalculateVertexSize(GraphicsTriShape->m_VertexDesc);
		uint32_t offset = 0;

		Data.pContext->IASetVertexBuffers(0, 1, &GraphicsTriShape->m_VertexBuffer, &stride, &offset);
		Data.pContext->IASetIndexBuffer(GraphicsTriShape->m_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		Data.pContext->DrawIndexed(Count * 3, StartIndex, 0);
	}

	void Renderer::DrawDynamicTriShapeUnknown(DynamicTriShape *Shape, DynamicTriShapeDrawData *DrawData, uint32_t IndexStartOffset, uint32_t TriangleCount)
	{
		DynamicTriShapeData shapeData;
		shapeData.m_VertexBuffer = Shape->m_VertexBuffer;
		shapeData.m_IndexBuffer = Shape->m_IndexBuffer;
		shapeData.m_VertexDesc = Shape->m_VertexDesc;

		DrawDynamicTriShape(&shapeData, DrawData, IndexStartOffset, TriangleCount, Shape->m_VertexAllocationOffset);
	}

	void Renderer::DrawDynamicTriShape(DynamicTriShapeData *ShapeData, DynamicTriShapeDrawData *DrawData, uint32_t IndexStartOffset, uint32_t TriangleCount, uint32_t VertexBufferOffset)
	{
		SetVertexDescription(ShapeData->m_VertexDesc);
		SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		SetDirtyStates(false);

		ID3D11Buffer *buffers[2];
		buffers[0] = ShapeData->m_VertexBuffer;
		buffers[1] = Globals.m_DynamicVertexBuffers[Globals.m_CurrentDynamicVertexBuffer];

		uint32_t strides[2];
		strides[0] = BSGeometry::CalculateVertexSize(ShapeData->m_VertexDesc);
		strides[1] = BSGeometry::CalculateDyanmicVertexSize(ShapeData->m_VertexDesc);

		uint32_t offsets[2];
		offsets[0] = 0;
		offsets[1] = VertexBufferOffset;

		Data.pContext->IASetVertexBuffers(0, 2, buffers, strides, offsets);
		Data.pContext->IASetIndexBuffer(ShapeData->m_IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		Data.pContext->DrawIndexed(TriangleCount * 3, IndexStartOffset, 0);
	}

	void Renderer::DrawParticleShaderTriShape(const void *DynamicData, uint32_t Count)
	{
		auto state = GetRendererShadowState();

		// Send dynamic data to GPU buffer
		uint32_t vertexStride = 48;
		uint32_t vertexOffset = 0;

		void *particleBuffer = AllocateAndMapDynamicVertexBuffer(vertexStride * Count, &vertexOffset);
		memcpy(particleBuffer, DynamicData, vertexStride * Count);
		UnmapDynamicTriShapeDynamicData(nullptr, nullptr);

		// Update flags but don't update the input layout - we use a custom one here
		SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		state->m_StateUpdateFlags &= ~DIRTY_VERTEX_DESC;

		SetDirtyStates(false);

		InputLayoutLock.lock();
		{
			if (!Globals.m_ParticleShaderInputLayout)
			{
				constexpr static D3D11_INPUT_ELEMENT_DESC inputDesc[] =
				{
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "TEXCOORD", 1, DXGI_FORMAT_R8G8B8A8_SINT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				};

				Assert(SUCCEEDED(Data.pDevice->CreateInputLayout(
					inputDesc,
					ARRAYSIZE(inputDesc),
					state->m_CurrentVertexShader->m_RawBytecode,
					state->m_CurrentVertexShader->m_ShaderLength,
					&Globals.m_ParticleShaderInputLayout)));
			}

			uint64_t desc = state->m_VertexDesc & state->m_CurrentVertexShader->m_VertexDescription;
			InputLayoutMap.try_emplace(desc, Globals.m_ParticleShaderInputLayout);
		}
		InputLayoutLock.unlock();

		Data.pContext->IASetInputLayout(Globals.m_ParticleShaderInputLayout);
		state->m_StateUpdateFlags |= DIRTY_VERTEX_DESC;

		Data.pContext->IASetIndexBuffer(Globals.m_SharedParticleIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
		Data.pContext->IASetVertexBuffers(0, 1, &Globals.m_DynamicVertexBuffers[Globals.m_CurrentDynamicVertexBuffer], &vertexStride, &vertexOffset);
		Data.pContext->DrawIndexed(6 * (Count / 4), 0, 0);
	}

	void Renderer::ClearColor()
	{
		Data.pContext->ClearRenderTargetView(Data.pRenderTargets[gRenderTargetManager.QCurrentPlatformRenderTarget()].RTV, Data.ClearColor);
	}

	void Renderer::ClearDepthStencil(uint32_t ClearFlags)
	{
		auto state = GetRendererShadowState();

		Data.bReadOnlyDepth = false;
		Data.pContext->ClearDepthStencilView(Data.pDepthStencils[state->m_DepthStencil].Views[state->m_DepthStencilSlice], ClearFlags, 1.0f, Data.ClearStencil);
	}

	DynamicTriShape *Renderer::GetParticlesDynamicTriShape()
	{
		static DynamicTriShape particles =
		{
			Globals.m_SharedParticleStaticBuffer,
			Globals.m_SharedParticleIndexBuffer,
			0x840200004000051,
			0xFFFFFFFF,
			0,
			1,
			nullptr,
			nullptr
		};

		return &particles;
	}

	void Renderer::SetDirtyStates(bool IsComputeShader)
	{
		auto rendererData = &Renderer::QInstance()->Data;
		auto state = Renderer::QInstance()->GetRendererShadowState();
		auto context = rendererData->pContext;

		if (uint32_t flags = state->m_StateUpdateFlags; flags != 0)
		{
			if (flags & DIRTY_RENDERTARGET)
			{
				// Build active render target view array
				ID3D11RenderTargetView *renderTargetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
				uint32_t viewCount = 0;

				if (state->m_CubeMapRenderTarget == RENDER_TARGET_CUBEMAP_NONE)
				{
					// This loops through all 8 RTs or until a RENDER_TARGET_NONE entry is hit
					for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
					{
						if (state->m_RenderTargets[i] == RENDER_TARGET_NONE)
							break;

						renderTargetViews[i] = rendererData->pRenderTargets[state->m_RenderTargets[i]].RTV;
						viewCount++;

						if (state->m_SetRenderTargetMode[i] == SRTM_CLEAR)
						{
							context->ClearRenderTargetView(renderTargetViews[i], rendererData->ClearColor);
							state->m_SetRenderTargetMode[i] = SRTM_NO_CLEAR;
						}
					}
				}
				else
				{
					// Use a single RT for the cubemap
					renderTargetViews[0] = rendererData->pCubemapRenderTargets[state->m_CubeMapRenderTarget].CubeSideRTV[state->m_CubeMapRenderTargetView];
					viewCount = 1;

					if (state->m_SetCubeMapRenderTargetMode == SRTM_CLEAR)
					{
						context->ClearRenderTargetView(renderTargetViews[0], rendererData->ClearColor);
						state->m_SetCubeMapRenderTargetMode = SRTM_NO_CLEAR;
					}
				}

				switch (state->m_SetDepthStencilMode)
				{
				case SRTM_CLEAR:
				case SRTM_CLEAR_DEPTH:
				case SRTM_CLEAR_STENCIL:
				case SRTM_INIT:
					rendererData->bReadOnlyDepth = false;
					break;
				}

				//
				// Determine which depth stencil to render to. When there's no active depth stencil,
				// simply send a nullptr to dx11.
				//
				ID3D11DepthStencilView *depthStencil = nullptr;

				if (state->m_DepthStencil != -1)
				{
					if (rendererData->bReadOnlyDepth)
						depthStencil = rendererData->pDepthStencils[state->m_DepthStencil].ReadOnlyViews[state->m_DepthStencilSlice];
					else
						depthStencil = rendererData->pDepthStencils[state->m_DepthStencil].Views[state->m_DepthStencilSlice];

					// Only clear the stencil if specific flags are set
					if (depthStencil)
					{
						uint32_t clearFlags = 0;

						switch (state->m_SetDepthStencilMode)
						{
						case SRTM_CLEAR:
						case SRTM_INIT:
							clearFlags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
							break;

						case SRTM_CLEAR_DEPTH:
							clearFlags = D3D11_CLEAR_DEPTH;
							break;

						case SRTM_CLEAR_STENCIL:
							clearFlags = D3D11_CLEAR_STENCIL;
							break;
						}

						if (clearFlags)
						{
							context->ClearDepthStencilView(depthStencil, clearFlags, 1.0f, 0);
							state->m_SetDepthStencilMode = SRTM_NO_CLEAR;
						}
					}
				}

				context->OMSetRenderTargets(viewCount, renderTargetViews, depthStencil);
			}

			// OMSetDepthStencilState
			if (flags & (DIRTY_DEPTH_STENCILREF_MODE | DIRTY_DEPTH_MODE))
			{
				context->OMSetDepthStencilState(Globals.m_DepthStates[state->m_DepthStencilDepthMode][state->m_DepthStencilStencilMode], state->m_StencilRef);
			}

			// RSSetState
			if (flags & (DIRTY_UNKNOWN2 | DIRTY_RASTER_DEPTH_BIAS | DIRTY_RASTER_CULL_MODE | DIRTY_UNKNOWN1))
			{
				context->RSSetState(Globals.m_RasterStates[state->m_RasterStateFillMode][state->m_RasterStateCullMode][state->m_RasterStateDepthBiasMode][state->m_RasterStateScissorMode]);

				if (flags & DIRTY_RASTER_DEPTH_BIAS)
				{
					if (state->m_ViewPort.MinDepth != state->m_CameraData.m_ViewDepthRange.x || state->m_ViewPort.MaxDepth != state->m_CameraData.m_ViewDepthRange.y)
					{
						state->m_ViewPort.MinDepth = state->m_CameraData.m_ViewDepthRange.x;
						state->m_ViewPort.MaxDepth = state->m_CameraData.m_ViewDepthRange.y;
						flags |= DIRTY_VIEWPORT;
					}

					if (state->m_RasterStateDepthBiasMode)
					{
						state->m_ViewPort.MaxDepth -= Globals.m_DepthBiasFactors[0][state->m_RasterStateDepthBiasMode];
						flags |= DIRTY_VIEWPORT;
					}
				}
			}

			// RSSetViewports
			if (flags & DIRTY_VIEWPORT)
			{
				context->RSSetViewports(1, &state->m_ViewPort);
			}

			// OMSetBlendState
			if (flags & DIRTY_ALPHA_BLEND)
			{
				const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

				context->OMSetBlendState(Globals.m_BlendStates[state->m_AlphaBlendMode][state->m_AlphaBlendAlphaToCoverage][state->m_AlphaBlendWriteMode][state->m_AlphaBlendModeExtra], blendFactor, 0xFFFFFFFF);
			}

			if (flags & (DIRTY_ALPHA_ENABLE_TEST | DIRTY_ALPHA_TEST_REF))
			{
				D3D11_MAPPED_SUBRESOURCE resource;
				context->Map(Globals.m_AlphaTestRefCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

				if (state->m_AlphaTestEnabled)
					*(float *)resource.pData = state->m_AlphaTestRef;
				else
					*(float *)resource.pData = 0.0f;

				context->Unmap(Globals.m_AlphaTestRefCB, 0);
			}

			// Shader input layout creation + updates
			if (!IsComputeShader && (flags & DIRTY_VERTEX_DESC))
			{
				InputLayoutLock.lock();
				{
					uint64_t desc = state->m_VertexDesc & state->m_CurrentVertexShader->m_VertexDescription;

					// Does the entry exist already?
					if (auto e = InputLayoutMap.find(desc); e != InputLayoutMap.end())
					{
						// It does. We're done now.
						context->IASetInputLayout(e->second);
					}
					else
					{
						// Create and insert
						AutoFunc(__int64(__fastcall *)(unsigned __int64 a1), sub_140D705F0, 0xD70620);
						ID3D11InputLayout *layout = (ID3D11InputLayout *)sub_140D705F0(desc);

						if (layout || desc != 0x300000000407)
							InputLayoutMap.emplace(desc, layout);

						context->IASetInputLayout(layout);
					}
				}
				InputLayoutLock.unlock();
			}

			// IASetPrimitiveTopology
			if (flags & DIRTY_PRIMITIVE_TOPO)
			{
				context->IASetPrimitiveTopology(state->m_Topology);
			}

			// Compute shaders are pretty much custom and always require state to be reset
			if (IsComputeShader)
				state->m_StateUpdateFlags = flags & DIRTY_PRIMITIVE_TOPO;
			else
				state->m_StateUpdateFlags = 0;
		}

		FlushD3DResources();
	}

	void Renderer::FlushD3DResources()
	{
		auto state = Renderer::QInstance()->GetRendererShadowState();
		auto context = Renderer::QInstance()->Data.pContext;

		//
		// Resource/state setting code. It's been modified to take 1 of 2 paths for each type:
		//
		// 1: modifiedBits == 0 { Do nothing }
		// 2: modifiedBits > 0  { Build minimal state change [X entries] before submitting it to DX }
		//
#define for_each_bit(Iterator, Bits) for (unsigned long Iterator; _BitScanForward(&Iterator, Bits); Bits &= ~(1 << Iterator))

		// Pixel shader samplers
		if (uint32_t bits = state->m_PSSamplerModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFF0000) == 0, "PSSamplerModifiedBits must not exceed 15th index");

			for_each_bit(i, bits)
				context->PSSetSamplers(i, 1, &Renderer::Globals.m_SamplerStates[state->m_PSTextureAddressMode[i]][state->m_PSTextureFilterMode[i]]);

			state->m_PSSamplerModifiedBits = 0;
		}

		// Pixel shader resources
		if (uint32_t bits = state->m_PSResourceModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFF0000) == 0, "PSResourceModifiedBits must not exceed 15th index");

			for_each_bit(i, bits)
			{
				// Combine PSSSR(0, 1, [rsc1]) + PSSSR(1, 1, [rsc2]) into PSSSR(0, 2, [rsc1, rsc2])
				if (bits & (1 << (i + 1)))
				{
					context->PSSetShaderResources(i, 2, &state->m_PSTexture[i]);
					bits &= ~(1 << (i + 1));
				}
				else
					context->PSSetShaderResources(i, 1, &state->m_PSTexture[i]);
			}

			state->m_PSResourceModifiedBits = 0;
		}

		// Compute shader samplers
		if (uint32_t bits = state->m_CSSamplerModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFF0000) == 0, "CSSamplerModifiedBits must not exceed 15th index");

			for_each_bit(i, bits)
				context->CSSetSamplers(i, 1, &Renderer::Globals.m_SamplerStates[state->m_CSTextureAddressMode[i]][state->m_CSTextureFilterMode[i]]);

			state->m_CSSamplerModifiedBits = 0;
		}

		// Compute shader resources
		if (uint32_t bits = state->m_CSResourceModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFF0000) == 0, "CSResourceModifiedBits must not exceed 15th index");

			for_each_bit(i, bits)
				context->CSSetShaderResources(i, 1, &state->m_CSTexture[i]);

			state->m_CSResourceModifiedBits = 0;
		}

		// Compute shader unordered access views (UAVs)
		if (uint32_t bits = state->m_CSUAVModifiedBits; bits != 0)
		{
			AssertMsg((bits & 0xFFFFFF00) == 0, "CSUAVModifiedBits must not exceed 7th index");

			for_each_bit(i, bits)
				context->CSSetUnorderedAccessViews(i, 1, &state->m_CSUAV[i], nullptr);

			state->m_CSUAVModifiedBits = 0;
		}

#undef for_each_bit
	}

	RendererShadowState *Renderer::GetRendererShadowState() const
	{
		return (RendererShadowState *)(g_ModuleBase + 0x304DEB0);
	}

	void BSGraphics::Renderer::SetRenderTarget(uint32_t Slot, uint32_t TargetIndex, SetRenderTargetMode Mode, bool UpdateViewport)
	{
		auto s = GetRendererShadowState();

		if (Mode == SRTM_FORCE_COPY_RESTORE)
		{
			Data.pContext->CopyResource(Data.pRenderTargets[TargetIndex].TextureCopy, Data.pRenderTargets[TargetIndex].Texture);
			Mode = SRTM_RESTORE;
		}

		if (s->m_RenderTargets[Slot] != TargetIndex || s->m_SetRenderTargetMode[Slot] != Mode || Mode != SRTM_RESTORE)
		{
			s->m_RenderTargets[Slot] = TargetIndex;
			s->m_CubeMapRenderTarget = RENDER_TARGET_CUBEMAP_NONE;
			s->m_SetRenderTargetMode[Slot] = Mode;
			s->m_StateUpdateFlags |= DIRTY_RENDERTARGET;
		}

		if (UpdateViewport)
			UpdateViewPort(0, 0, false);
	}

	void BSGraphics::Renderer::SetDepthStencilTarget(uint32_t TargetIndex, SetRenderTargetMode Mode, uint32_t Slice)
	{
		auto s = GetRendererShadowState();

		if (s->m_DepthStencil != TargetIndex || s->m_SetDepthStencilMode != Mode || Mode != SRTM_RESTORE || s->m_DepthStencilSlice != Slice)
		{
			s->m_DepthStencil = TargetIndex;
			s->m_SetDepthStencilMode = Mode;
			s->m_DepthStencilSlice = Slice;
			s->m_StateUpdateFlags |= DIRTY_RENDERTARGET;
		}
	}

	void BSGraphics::Renderer::SetCubeMapRenderTarget(uint32_t TargetIndex, SetRenderTargetMode Mode, uint32_t View, bool UpdateViewport)
	{
		auto s = GetRendererShadowState();

		if (s->m_CubeMapRenderTarget != TargetIndex || s->m_SetCubeMapRenderTargetMode != Mode || s->m_CubeMapRenderTargetView != View)
		{
			s->m_CubeMapRenderTarget = TargetIndex;
			s->m_SetCubeMapRenderTargetMode = Mode;
			s->m_CubeMapRenderTargetView = View;
			s->m_StateUpdateFlags |= DIRTY_RENDERTARGET;
		}

		if (UpdateViewport)
			UpdateViewPort(0, 0, false);
	}

	void BSGraphics::Renderer::SetClearColor(float R, float G, float B, float A)
	{
		auto renderer = Renderer::QInstance();

		Globals.m_PreviousClearColor[0] = renderer->Data.ClearColor[0];
		Globals.m_PreviousClearColor[1] = renderer->Data.ClearColor[1];
		Globals.m_PreviousClearColor[2] = renderer->Data.ClearColor[2];
		Globals.m_PreviousClearColor[3] = renderer->Data.ClearColor[3];

		renderer->Data.ClearColor[0] = R;
		renderer->Data.ClearColor[1] = G;
		renderer->Data.ClearColor[2] = B;
		renderer->Data.ClearColor[3] = A;
	}

	void BSGraphics::Renderer::RestorePreviousClearColor()
	{
		auto renderer = Renderer::QInstance();

		renderer->Data.ClearColor[0] = Globals.m_PreviousClearColor[0];
		renderer->Data.ClearColor[1] = Globals.m_PreviousClearColor[1];
		renderer->Data.ClearColor[2] = Globals.m_PreviousClearColor[2];
		renderer->Data.ClearColor[3] = Globals.m_PreviousClearColor[3];
	}

	void Renderer::UpdateViewPort(uint32_t Width, uint32_t Height, bool ForceMatchRenderTarget)
	{
		auto s = GetRendererShadowState();

		float widthRatio = 1.0f;
		float heightRatio = 1.0f;

		if (Width <= 0)
		{
			if (!ForceMatchRenderTarget)
			{
				if (gState.uiDynamicResolutionUnknown2)
				{
					widthRatio = 1.0f;
					heightRatio = 1.0f;
				}
				else
				{
					widthRatio = gState.fDynamicResolutionWidthRatio;
					heightRatio = gState.fDynamicResolutionHeightRatio;
				}
			}

			if (s->m_CubeMapRenderTarget != RENDER_TARGET_CUBEMAP_NONE)
			{
				Width = gRenderTargetManager.QCurrentCubeMapRenderTargetWidth();
				Height = gRenderTargetManager.QCurrentCubeMapRenderTargetHeight();
			}
			else
			{
				Width = gRenderTargetManager.QCurrentRenderTargetWidth();
				Height = gRenderTargetManager.QCurrentRenderTargetHeight();
			}
		}

		// TODO: Does viewport depth need to be set here? SSE doesn't but F4 does.
		s->m_ViewPort.TopLeftX = s->m_CameraData.m_ViewPort[0] * Width;
		s->m_ViewPort.TopLeftY = (1.0f - s->m_CameraData.m_ViewPort[2]) * Height;
		s->m_ViewPort.Width = (s->m_CameraData.m_ViewPort[1] - s->m_CameraData.m_ViewPort[0]) * Width * widthRatio;
		s->m_ViewPort.Height = (s->m_CameraData.m_ViewPort[2] - s->m_CameraData.m_ViewPort[3]) * Height * heightRatio;
		//s->m_ViewPort.MinDepth = s->m_CameraData.m_ViewDepthRange[0];
		//s->m_ViewPort.MaxDepth = s->m_CameraData.m_ViewDepthRange[1];

		s->m_StateUpdateFlags |= DIRTY_VIEWPORT;
	}

	void Renderer::DepthStencilStateSetDepthMode(DepthStencilDepthMode Mode)
	{
		auto s = GetRendererShadowState();

		if (s->m_DepthStencilDepthMode != Mode)
		{
			s->m_DepthStencilDepthMode = Mode;

			// Temp var to prevent duplicate state setting? Don't know where this gets set.
			if (s->m_DepthStencilUnknown != Mode)
				s->m_StateUpdateFlags |= DIRTY_DEPTH_MODE;
			else
				s->m_StateUpdateFlags &= ~DIRTY_DEPTH_MODE;
		}
	}

	DepthStencilDepthMode Renderer::DepthStencilStateGetDepthMode() const
	{
		return GetRendererShadowState()->m_DepthStencilDepthMode;
	}

	void Renderer::DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef)
	{
		auto s = GetRendererShadowState();

		if (s->m_DepthStencilStencilMode != Mode || s->m_StencilRef != StencilRef)
		{
			s->m_DepthStencilStencilMode = Mode;
			s->m_StencilRef = StencilRef;
			s->m_StateUpdateFlags |= DIRTY_DEPTH_STENCILREF_MODE;
		}
	}

	void Renderer::RasterStateSetCullMode(uint32_t CullMode)
	{
		auto s = GetRendererShadowState();

		if (s->m_RasterStateCullMode != CullMode)
		{
			s->m_RasterStateCullMode = CullMode;
			s->m_StateUpdateFlags |= DIRTY_RASTER_CULL_MODE;
		}
	}

	void Renderer::RasterStateSetDepthBias(uint32_t Value)
	{
		auto s = GetRendererShadowState();

		if (s->m_RasterStateDepthBiasMode != Value)
		{
			s->m_RasterStateDepthBiasMode = Value;
			s->m_StateUpdateFlags |= DIRTY_RASTER_DEPTH_BIAS;
		}
	}

	void Renderer::AlphaBlendStateSetMode(uint32_t Mode)
	{
		auto s = GetRendererShadowState();

		if (s->m_AlphaBlendMode != Mode)
		{
			s->m_AlphaBlendMode = Mode;
			s->m_StateUpdateFlags |= DIRTY_ALPHA_BLEND;
		}
	}

	void Renderer::AlphaBlendStateSetAlphaToCoverage(uint32_t Value)
	{
		auto s = GetRendererShadowState();

		if (s->m_AlphaBlendAlphaToCoverage != Value)
		{
			s->m_AlphaBlendAlphaToCoverage = Value;
			s->m_StateUpdateFlags |= DIRTY_ALPHA_BLEND;
		}
	}

	void Renderer::AlphaBlendStateSetWriteMode(uint32_t Value)
	{
		auto s = GetRendererShadowState();

		if (s->m_AlphaBlendWriteMode != Value)
		{
			s->m_AlphaBlendWriteMode = Value;
			s->m_StateUpdateFlags |= DIRTY_ALPHA_BLEND;
		}
	}

	uint32_t Renderer::AlphaBlendStateGetWriteMode() const
	{
		return GetRendererShadowState()->m_AlphaBlendWriteMode;
	}

	void Renderer::SetUseAlphaTestRef(bool UseStoredValue)
	{
		auto s = GetRendererShadowState();

		// When UseStoredValue is false, the constant buffer data is zeroed, but m_AlphaTestRef is saved
		if (s->m_AlphaTestEnabled != UseStoredValue)
		{
			s->m_AlphaTestEnabled = UseStoredValue;
			s->m_StateUpdateFlags |= DIRTY_ALPHA_TEST_REF;
		}
	}

	void Renderer::SetAlphaTestRef(float Value)
	{
		auto s = GetRendererShadowState();

		if (s->m_AlphaTestRef != Value)
		{
			s->m_AlphaTestRef = Value;
			s->m_StateUpdateFlags |= DIRTY_ALPHA_ENABLE_TEST;
		}
	}

	void Renderer::SetVertexDescription(uint64_t VertexDesc)
	{
		auto s = GetRendererShadowState();

		if (s->m_VertexDesc != VertexDesc)
		{
			s->m_VertexDesc = VertexDesc;
			s->m_StateUpdateFlags |= DIRTY_VERTEX_DESC;
		}
	}

	void Renderer::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology)
	{
		auto s = GetRendererShadowState();

		if (s->m_Topology != Topology)
		{
			s->m_Topology = Topology;
			s->m_StateUpdateFlags |= DIRTY_PRIMITIVE_TOPO;
		}
	}

	void Renderer::SetVertexShader(VertexShader *Shader)
	{
		auto s = GetRendererShadowState();

		// The input layout (IASetInputLayout) may need to be created and updated
		s->m_CurrentVertexShader = Shader;
		Data.pContext->VSSetShader(Shader ? Shader->m_Shader : nullptr, nullptr, 0);
		s->m_StateUpdateFlags |= DIRTY_VERTEX_DESC;
	}

	void Renderer::SetPixelShader(PixelShader *Shader)
	{
		auto s = GetRendererShadowState();

		s->m_CurrentPixelShader = Shader;
		Data.pContext->PSSetShader(Shader ? Shader->m_Shader : nullptr, nullptr, 0);
	}

	void Renderer::SetHullShader(HullShader *Shader)
	{
		Data.pContext->HSSetShader(Shader ? Shader->m_Shader : nullptr, nullptr, 0);
	}

	void Renderer::SetDomainShader(DomainShader *Shader)
	{
		Data.pContext->DSSetShader(Shader ? Shader->m_Shader : nullptr, nullptr, 0);
	}

	void Renderer::SetTexture(uint32_t Index, const NiSourceTexture *Texture)
	{
		SetTexture(Index, Texture ? Texture->QRendererTexture() : nullptr);
	}

	void Renderer::SetTexture(uint32_t Index, const Texture *Resource)
	{
		SetShaderResource(Index, Resource ? Resource->m_ResourceView : nullptr);
	}

	void Renderer::SetTextureMode(uint32_t Index, uint32_t AddressMode, uint32_t FilterMode)
	{
		SetTextureAddressMode(Index, AddressMode);
		SetTextureFilterMode(Index, FilterMode);
	}

	void Renderer::SetTextureAddressMode(uint32_t Index, uint32_t Mode)
	{
		auto s = GetRendererShadowState();

		if (s->m_PSTextureAddressMode[Index] != Mode)
		{
			s->m_PSTextureAddressMode[Index] = Mode;
			s->m_PSSamplerModifiedBits |= 1 << Index;
		}
	}

	void Renderer::SetTextureFilterMode(uint32_t Index, uint32_t Mode)
	{
		auto s = GetRendererShadowState();

		if (s->m_PSTextureFilterMode[Index] != Mode)
		{
			s->m_PSTextureFilterMode[Index] = Mode;
			s->m_PSSamplerModifiedBits |= 1 << Index;
		}
	}

	void BSGraphics::Renderer::SetShaderResource(uint32_t Index, ID3D11ShaderResourceView *Resource)
	{
		auto s = GetRendererShadowState();

		if (s->m_PSTexture[Index] != Resource)
		{
			s->m_PSTexture[Index] = Resource;
			s->m_PSResourceModifiedBits |= 1 << Index;
		}
	}

	ComPtr<ID3DBlob> Renderer::CompileShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, const char *ProgramType)
	{
		// Build defines (aka convert vector->D3DCONSTANT array)
		std::vector<D3D_SHADER_MACRO> macros;

		for (auto& i : Defines)
			macros.push_back({ i.first, i.second });

		if (!_stricmp(ProgramType, "ps_5_0"))
			macros.push_back({ "PIXELSHADER", "" });
		else if (!_stricmp(ProgramType, "vs_5_0"))
			macros.push_back({ "VERTEXSHADER", "" });
		else if (!_stricmp(ProgramType, "hs_5_0"))
			macros.push_back({ "HULLSHADER", "" });
		else if (!_stricmp(ProgramType, "ds_5_0"))
			macros.push_back({ "DOMAINSHADER", "" });
		else if (!_stricmp(ProgramType, "cs_5_0"))
			macros.push_back({ "COMPUTESHADER", "" });
		else
			Assert(false);

		// Add null terminating entry
		macros.push_back({ "WINPC", "" });
		macros.push_back({ "DX11", "" });
		macros.push_back({ nullptr, nullptr });

		// Compiler setup
		uint32_t flags = D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
		ComPtr<ID3DBlob> shaderBlob;
		ComPtr<ID3DBlob> shaderErrors;

		if (FAILED(D3DCompileFromFile(FilePath, macros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", ProgramType, flags, 0, &shaderBlob, &shaderErrors)))
		{
			AssertMsgVa(false, "Shader compilation failed:\n\n%s", shaderErrors ? (const char *)shaderErrors->GetBufferPointer() : "Unknown error");
			return nullptr;
		}

		return shaderBlob;
	}

	VertexShader *Renderer::CompileVertexShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetConstant)
	{
		auto shaderBlob = CompileShader(FilePath, Defines, "vs_5_0");

		if (!shaderBlob)
			return nullptr;

		void *rawPtr = malloc(sizeof(VertexShader) + shaderBlob->GetBufferSize());
		VertexShader *vs = new (rawPtr) VertexShader;

		// Shader reflection: gather constant buffer variable offsets
		ComPtr<ID3D11ShaderReflection> reflector;
		Assert(SUCCEEDED(D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_PPV_ARGS(&reflector))));

		ReflectConstantBuffers(reflector, vs->m_ConstantGroups, ARRAYSIZE(vs->m_ConstantGroups), GetConstant, vs->m_ConstantOffsets, ARRAYSIZE(vs->m_ConstantOffsets));

		// Register shader with the DX runtime itself
		Assert(SUCCEEDED(Data.pDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &vs->m_Shader)));

		// Final step: append raw bytecode to the end of the struct
		memcpy(vs->m_RawBytecode, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
		vs->m_ShaderLength = (uint32_t)shaderBlob->GetBufferSize();

		RegisterShaderBytecode(vs->m_Shader, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
		return vs;
	}

	PixelShader *Renderer::CompilePixelShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetSampler, std::function<const char *(int Index)> GetConstant)
	{
		auto shaderBlob = CompileShader(FilePath, Defines, "ps_5_0");

		if (!shaderBlob)
			return nullptr;

		PixelShader *ps = new PixelShader;

		// Shader reflection: gather constant buffer variable offsets and check for valid sampler mappings
		ComPtr<ID3D11ShaderReflection> reflector;
		Assert(SUCCEEDED(D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_PPV_ARGS(&reflector))));

		ReflectConstantBuffers(reflector, ps->m_ConstantGroups, ARRAYSIZE(ps->m_ConstantGroups), GetConstant, ps->m_ConstantOffsets, ARRAYSIZE(ps->m_ConstantOffsets));
		ReflectSamplers(reflector, GetSampler);

		// Register shader with the DX runtime itself
		Assert(SUCCEEDED(Data.pDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &ps->m_Shader)));

		RegisterShaderBytecode(ps->m_Shader, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
		return ps;
	}

	HullShader *Renderer::CompileHullShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines)
	{
		auto shaderBlob = CompileShader(FilePath, Defines, "hs_5_0");

		if (!shaderBlob)
			return nullptr;

		HullShader *hs = new HullShader;

		// Register shader with the DX runtime itself
		Assert(SUCCEEDED(Data.pDevice->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &hs->m_Shader)));

		RegisterShaderBytecode(hs->m_Shader, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
		return hs;
	}

	DomainShader *Renderer::CompileDomainShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines)
	{
		auto shaderBlob = CompileShader(FilePath, Defines, "ds_5_0");

		if (!shaderBlob)
			return nullptr;

		DomainShader *ds = new DomainShader;

		// Register shader with the DX runtime itself
		Assert(SUCCEEDED(Data.pDevice->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &ds->m_Shader)));

		RegisterShaderBytecode(ds->m_Shader, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());
		return ds;
	}

	void Renderer::ReflectConstantBuffers(ComPtr<ID3D11ShaderReflection> Reflector, Buffer *ConstantGroups, uint32_t MaxGroups, std::function<const char *(int Index)> GetConstant, uint8_t *Offsets, uint32_t MaxOffsets)
	{
		D3D11_SHADER_DESC desc;
		Assert(SUCCEEDED(Reflector->GetDesc(&desc)));

		// These should always be cleared first - invalid offsets don't get sent to the GPU
		memset(Offsets, INVALID_CONSTANT_BUFFER_OFFSET, MaxOffsets * sizeof(uint8_t));
		memset(ConstantGroups, 0, MaxGroups * sizeof(Buffer));

		if (desc.ConstantBuffers <= 0)
			return;

		auto mapBufferConsts = [&](ID3D11ShaderReflectionConstantBuffer *Reflector, Buffer *ConstantGroup)
		{
			// If this call fails, it's an invalid buffer
			D3D11_SHADER_BUFFER_DESC bufferDesc;
			if (FAILED(Reflector->GetDesc(&bufferDesc)))
				return;

			for (uint32_t i = 0; i < bufferDesc.Variables; i++)
			{
				ID3D11ShaderReflectionVariable *var = Reflector->GetVariableByIndex(i);

				D3D11_SHADER_VARIABLE_DESC varDesc;
				Assert(SUCCEEDED(var->GetDesc(&varDesc)));

				AssertMsgVa(varDesc.StartOffset % 4 == 0, "Variable '%s' is not aligned to 4", varDesc.Name);

				// Variable name maps to hardcoded index in SSE executable
				for (int j = 0;; j++)
				{
					const char *hardConstName = GetConstant(j);

					if (!hardConstName)
						break;

					if (_stricmp(hardConstName, varDesc.Name) == 0)
					{
						// Found!
						Offsets[j] = varDesc.StartOffset / 4;
						var = nullptr;
					}
				}

				AssertMsgVa(var == nullptr, "Variable '%s' did not have an index mapping in the executable", varDesc.Name);
			}

			// Nasty type cast here, but it's how the game does it (round up to nearest 16 bytes)
			*(uintptr_t *)&ConstantGroup->m_Buffer = (bufferDesc.Size + 15) & ~15;
		};

		// Each buffer is optional (nullptr if nonexistent)
		Assert(MaxGroups == 3);

		//mapBufferConsts(Reflector->GetConstantBufferByName("PerTechnique"), &ConstantGroups[0]);
		//mapBufferConsts(Reflector->GetConstantBufferByName("PerMaterial"), &ConstantGroups[1]);
		//mapBufferConsts(Reflector->GetConstantBufferByName("PerGeometry"), &ConstantGroups[2]);
	}

	void Renderer::ReflectSamplers(ComPtr<ID3D11ShaderReflection> Reflector, std::function<const char *(int Index)> GetSampler)
	{
		D3D11_SHADER_DESC desc;
		Assert(SUCCEEDED(Reflector->GetDesc(&desc)));

		if (desc.BoundResources <= 0)
			return;

		// Loop through all shader resources, then pick out sampler types specifically
		for (uint32_t i = 0; i < desc.BoundResources; i++)
		{
			D3D11_SHADER_INPUT_BIND_DESC inputDesc;

			if (FAILED(Reflector->GetResourceBindingDesc(i, &inputDesc)))
				continue;

			if (inputDesc.Type != D3D_SIT_SAMPLER)
				continue;

			// Do a partial string match
			const char *ourSamplerName = GetSampler(inputDesc.BindPoint);
			const char *dxSamplerName = inputDesc.Name;

			AssertMsgVa(_strnicmp(ourSamplerName, dxSamplerName, strlen(ourSamplerName)) == 0, "Sampler names don't match (%s != %s)", ourSamplerName, dxSamplerName);
		}
	}

	void Renderer::ValidateShaderReplacement(ID3D11PixelShader *Original, ID3D11PixelShader *Replacement)
	{
		ValidateShaderReplacement(Original, Replacement, __uuidof(ID3D11PixelShader));
	}

	void Renderer::ValidateShaderReplacement(ID3D11VertexShader *Original, ID3D11VertexShader *Replacement)
	{
		ValidateShaderReplacement(Original, Replacement, __uuidof(ID3D11VertexShader));
	}

	void Renderer::ValidateShaderReplacement(ID3D11ComputeShader *Original, ID3D11ComputeShader *Replacement)
	{
		ValidateShaderReplacement(Original, Replacement, __uuidof(ID3D11ComputeShader));
	}

	void Renderer::ValidateShaderReplacement(void *Original, void *Replacement, const GUID& Guid)
	{
#if 0
		// First get the shader<->bytecode entry
		const auto& oldData = m_ShaderBytecodeMap.find(Original);
		const auto& newData = m_ShaderBytecodeMap.find(Replacement);

		Assert(oldData != m_ShaderBytecodeMap.end() && newData != m_ShaderBytecodeMap.end());

		// Disassemble both shaders, then compare the string output (case insensitive)
		UINT stripFlags = D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS | D3DCOMPILER_STRIP_PRIVATE_DATA;
		ComPtr<ID3DBlob> oldStrippedBlob;
		ComPtr<ID3DBlob> newStrippedBlob;

		Assert(SUCCEEDED(D3DStripShader(oldData->second.first, oldData->second.second, stripFlags, &oldStrippedBlob)));
		Assert(SUCCEEDED(D3DStripShader(newData->second.first, newData->second.second, stripFlags, &newStrippedBlob)));

		UINT disasmFlags = D3D_DISASM_ENABLE_INSTRUCTION_OFFSET;
		ComPtr<ID3DBlob> oldDataBlob;
		ComPtr<ID3DBlob> newDataBlob;

		Assert(SUCCEEDED(D3DDisassemble(oldStrippedBlob->GetBufferPointer(), oldStrippedBlob->GetBufferSize(), disasmFlags, nullptr, &oldDataBlob)));
		Assert(SUCCEEDED(D3DDisassemble(newStrippedBlob->GetBufferPointer(), newStrippedBlob->GetBufferSize(), disasmFlags, nullptr, &newDataBlob)));

		const char *oldDataStr = (const char *)oldDataBlob->GetBufferPointer();
		const char *newDataStr = (const char *)newDataBlob->GetBufferPointer();

		// Split the strings into multiple lines for better feedback info. Also skip some debug information
		// at the top of the file.
		auto tokenize = [](const std::string& str, std::vector<std::string> *tokens)
		{
			auto lastPos = str.find_first_not_of("\n", 0);
			auto pos = str.find_first_of("\n", lastPos);

			while (pos != std::string::npos || lastPos != std::string::npos)
			{
				tokens->push_back(str.substr(lastPos, pos - lastPos));
				lastPos = str.find_first_not_of("\n", pos);
				pos = str.find_first_of("\n", lastPos);
			}
		};

		std::vector<std::string> tokensOld;
		tokenize(std::string(oldDataStr, oldDataStr + oldDataBlob->GetBufferSize()), &tokensOld);

		std::vector<std::string> tokensNew;
		tokenize(std::string(newDataStr, newDataStr + newDataBlob->GetBufferSize()), &tokensNew);

		//Assert(tokensOld.size() == tokensNew.size());

		for (size_t i = 0; i < tokensOld.size() && i < tokensNew.size(); i++)
		{
			// Does the line match 1:1?
			if (_stricmp(tokensOld[i].c_str(), tokensNew[i].c_str()) == 0)
				continue;

			if (!strstr(tokensOld[i].c_str(), "//"))
				continue;

			// Skip "Approximately X instruction slots used" which is not always accurate.
			// Skip "dcl_constantbuffer" which is not always in order.
			if (strstr(tokensOld[i].c_str(), "// Approximately") || strstr(tokensOld[i].c_str(), "dcl_constantbuffer"))
				continue;

			AssertMsgVa(false, "Shader disasm doesn't match.\n\n%s\n%s", tokensOld[i].c_str(), tokensNew[i].c_str());
		}
#endif
	}

	void Renderer::RegisterShaderBytecode(void *Shader, const void *Bytecode, size_t BytecodeLength)
	{
		// Grab a copy since the pointer isn't going to be valid forever
		auto codeCopy = std::make_unique<uint8_t[]>(BytecodeLength);
		memcpy(codeCopy.get(), Bytecode, BytecodeLength);

		ShaderBytecodeMap.emplace(Shader, std::make_pair(std::move(codeCopy), BytecodeLength));
	}

	const std::pair<std::unique_ptr<uint8_t[]>, size_t>& Renderer::GetShaderBytecode(void *Shader)
	{
		return ShaderBytecodeMap.at(Shader);
	}

	void *Renderer::AllocateAndMapDynamicVertexBuffer(uint32_t Size, uint32_t *OutOffset)
	{
		AssertMsg(Size > 0, "Size must be > 0");

		uint32_t frameDataOffset = Globals.m_CurrentDynamicVertexBufferOffset;
		uint32_t frameBufferIndex = Globals.m_CurrentDynamicVertexBuffer;
		uint32_t newFrameDataSzie = Globals.m_CurrentDynamicVertexBufferOffset + Size;

		//
		// Check if this request would exceed the allocated buffer size for the currently executing command list. If it does,
		// we end the current query and move on to the next buffer.
		//
		if (newFrameDataSzie > 0x400000)
		{
			AssertMsg(Size <= 0x400000, "Dynamic geometry buffer overflow.");

			newFrameDataSzie = Size;
			frameDataOffset = 0;

			Globals.m_DynamicEventQueryFinished[Globals.m_CurrentDynamicVertexBuffer] = false;
			Data.pContext->End(Globals.m_DynamicVertexBufferAvailQuery[Globals.m_CurrentDynamicVertexBuffer]);

			frameBufferIndex++;

			if (frameBufferIndex >= ARRAYSIZE(Globals.m_DynamicVertexBufferAvailQuery))
				frameBufferIndex = 0;
		}

		//
		// This will suspend execution until the buffer we want is no longer in use. The query waits on a list of commands
		// using said buffer.
		//
		if (!Globals.m_DynamicEventQueryFinished[frameBufferIndex])
		{
			ID3D11Query *query = Globals.m_DynamicVertexBufferAvailQuery[frameBufferIndex];
			BOOL data;

			HRESULT hr = Data.pContext->GetData(query, &data, sizeof(data), 0);

			for (; FAILED(hr) || data == FALSE; hr = Data.pContext->GetData(query, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH))
				Sleep(1);

			Globals.m_DynamicEventQueryFinished[frameBufferIndex] = (data == TRUE);
		}

		D3D11_MAPPED_SUBRESOURCE resource;
		Data.pContext->Map(Globals.m_DynamicVertexBuffers[frameBufferIndex], 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &resource);

		Globals.m_CurrentDynamicVertexBuffer = frameBufferIndex;
		Globals.m_CurrentDynamicVertexBufferOffset = newFrameDataSzie;
		*OutOffset = frameDataOffset;

		return (void *)((uintptr_t)resource.pData + frameDataOffset);
	}

	void BSGraphics::Renderer::UnmapDynamicVertexBuffer()
	{
		Data.pContext->Unmap(Globals.m_DynamicVertexBuffers[Globals.m_CurrentDynamicVertexBuffer], 0);
	}

	void *Renderer::MapDynamicTriShapeDynamicData(BSDynamicTriShape *DynTriShape, DynamicTriShape *TriShape, DynamicTriShapeDrawData *DrawData, uint32_t VertexSize)
	{
		if (VertexSize <= 0)
			VertexSize = TriShape->m_VertexAllocationSize;

		return AllocateAndMapDynamicVertexBuffer(VertexSize, &TriShape->m_VertexAllocationOffset);
	}

	void Renderer::UnmapDynamicTriShapeDynamicData(DynamicTriShape *TriShape, DynamicTriShapeDrawData *DrawData)
	{
		UnmapDynamicVertexBuffer();
	}

	CustomConstantGroup Renderer::GetShaderConstantGroup(uint32_t Size, ConstantGroupLevel Level)
	{
		CustomConstantGroup temp;
		temp.m_Buffer = ShaderConstantBuffer->D3DBuffer;
		temp.m_Map.pData = ShaderConstantBuffer->MapData(Data.pContext, Size, &temp.m_UnifiedByteOffset, false);
		temp.m_Map.DepthPitch = Size;
		temp.m_Map.RowPitch = Size;

		memset(temp.m_Map.pData, 0, Size);
		return temp;
	}

	VertexCGroup Renderer::GetShaderConstantGroup(VertexShader *Shader, ConstantGroupLevel Level)
	{
		ConstantGroup<VertexShader> temp;
		Buffer *group = &Shader->m_ConstantGroups[Level];

		if (group->m_Buffer)
		{
			D3D11_BUFFER_DESC desc;

			if ((uintptr_t)group->m_Buffer > 0x10000)
			{
				group->m_Buffer->GetDesc(&desc);
				group->m_Buffer = (ID3D11Buffer *)desc.ByteWidth;
			}
			else
			{
				desc.ByteWidth = (uint32_t)group->m_Buffer;
			}

			temp = GetShaderConstantGroup(desc.ByteWidth, Level);
		}
		else
		{
			temp.m_Map.pData = group->m_Data;
			// Size to memset() is unknown here
		}

		temp.m_Shader = Shader;
		return temp;
	}

	PixelCGroup Renderer::GetShaderConstantGroup(PixelShader *Shader, ConstantGroupLevel Level)
	{
		ConstantGroup<PixelShader> temp;
		Buffer *group = &Shader->m_ConstantGroups[Level];

		if (group->m_Buffer)
		{
			D3D11_BUFFER_DESC desc;

			if ((uintptr_t)group->m_Buffer > 0x10000)
			{
				group->m_Buffer->GetDesc(&desc);
				group->m_Buffer = (ID3D11Buffer *)desc.ByteWidth;
			}
			else
			{
				desc.ByteWidth = (uint32_t)group->m_Buffer;
			}

			temp = GetShaderConstantGroup(desc.ByteWidth, Level);
		}
		else
		{
			temp.m_Map.pData = group->m_Data;
			// Size to memset() is unknown here
		}

		temp.m_Shader = Shader;
		return temp;
	}

	void Renderer::FlushConstantGroup(CustomConstantGroup *Group)
	{
		// Invalidate the data pointer only - ApplyConstantGroup still needs RowPitch info
		Group->m_Map.pData = (void *)0xFEFEFEFEFEFEFEFE;
	}

	void Renderer::FlushConstantGroupVSPS(ConstantGroup<VertexShader> *VertexGroup, ConstantGroup<PixelShader> *PixelGroup)
	{
		if (VertexGroup)
			FlushConstantGroup(VertexGroup);

		if (PixelGroup)
			FlushConstantGroup(PixelGroup);
	}

	void Renderer::ApplyConstantGroupVS(const CustomConstantGroup *Group, ConstantGroupLevel Level)
	{
		uint32_t offset = Group->m_UnifiedByteOffset / 16;
		uint32_t size = Group->m_Map.RowPitch / 16;

		Data.pContext->VSSetConstantBuffers1(Level, 1, &Group->m_Buffer, &offset, &size);
		Data.pContext->DSSetConstantBuffers1(Level, 1, &Group->m_Buffer, &offset, &size);
	}

	void Renderer::ApplyConstantGroupPS(const CustomConstantGroup *Group, ConstantGroupLevel Level)
	{
		uint32_t offset = Group->m_UnifiedByteOffset / 16;
		uint32_t size = Group->m_Map.RowPitch / 16;

		Data.pContext->PSSetConstantBuffers1(Level, 1, &Group->m_Buffer, &offset, &size);
	}

	void Renderer::ApplyConstantGroupVSPS(const ConstantGroup<VertexShader> *VertexGroup, const ConstantGroup<PixelShader> *PixelGroup, ConstantGroupLevel Level)
	{
		if (VertexGroup)
			ApplyConstantGroupVS(VertexGroup, Level);

		if (PixelGroup)
			ApplyConstantGroupPS(PixelGroup, Level);
	}

	void Renderer::IncRef(TriShape *Shape)
	{
		InterlockedIncrement(&Shape->m_RefCount);
	}

	void Renderer::DecRef(TriShape *Shape)
	{
		if (InterlockedDecrement(&Shape->m_RefCount) == 0)
		{
			if (Shape->m_VertexBuffer)
				Shape->m_VertexBuffer->Release();

			if (Shape->m_IndexBuffer)
				Shape->m_IndexBuffer->Release();

			AutoFunc(void(__fastcall *)(void *), sub_1400F7DC0, 0xF7DC0);
			AutoFunc(void(__fastcall *)(void *, __int64), sub_140136112C, 0x136112C);

			if (Shape->m_RawVertexData)
				sub_1400F7DC0(Shape->m_RawVertexData);

			if (Shape->m_RawIndexData)
				sub_1400F7DC0(Shape->m_RawIndexData);

			sub_140136112C(Shape, sizeof(TriShape));
		}
	}

	void Renderer::IncRef(DynamicTriShape *Shape)
	{
		InterlockedIncrement(&Shape->m_RefCount);
	}

	void Renderer::DecRef(DynamicTriShape *Shape)
	{
		if (InterlockedDecrement(&Shape->m_RefCount) == 0)
		{
			if (Shape->m_VertexBuffer)
				Shape->m_VertexBuffer->Release();

			Shape->m_IndexBuffer->Release();

			AutoFunc(void(__fastcall *)(void *), sub_1400F7DC0, 0xF7DC0);
			AutoFunc(void(__fastcall *)(void *, __int64), sub_140136112C, 0x136112C);

			sub_1400F7DC0(Shape->m_RawIndexData);
			sub_140136112C(Shape, sizeof(DynamicTriShape));
		}
	}
}