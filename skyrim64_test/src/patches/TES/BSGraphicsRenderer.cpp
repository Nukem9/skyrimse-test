#include "../rendering/common.h"
#include "../rendering/GpuCircularBuffer.h"
#include "BSGraphicsRenderer.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSVertexShader.h"
#include "BSShader/BSPixelShader.h"
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
}

namespace BSGraphics
{
	int CurrentFrameIndex;

	const uint32_t VertexIndexRingBufferSize = 32 * 1024 * 1024;
	const uint32_t ShaderConstantRingBufferSize = 32 * 1024 * 1024;

	thread_local BSVertexShader *TLS_CurrentVertexShader;
	thread_local BSPixelShader *TLS_CurrentPixelShader;

	GpuCircularBuffer DynamicBuffer;		// Holds vertices and indices
	GpuCircularBuffer ShaderConstantBuffer;	// Holds shader constant values

	ID3D11Query *FrameCompletedQueries[16];
	bool FrameCompletedQueryPending[16];

	thread_local uint64_t TestBufferUsedBits[4];
	ID3D11Buffer *TestBuffers[4][64];
	ID3D11Buffer *TestLargeBuffer;

	ID3D11Buffer *TempDynamicBuffers[11];

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

		uint32_t maxFrames = 16;

		for (int i = 0; i < maxFrames; i++)
		{
			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_EVENT;
			desc.MiscFlags = 0;

			if (FAILED(renderer->m_Device->CreateQuery(&desc, &FrameCompletedQueries[i])))
				__debugbreak();
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

			if (FAILED(renderer->m_Device->CreateBuffer(&desc, nullptr, &TempDynamicBuffers[i])))
				__debugbreak();
		}

		DynamicBuffer.Initialize(renderer->m_Device, D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER, VertexIndexRingBufferSize, maxFrames);

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
			{
				if (FAILED(renderer->m_Device->CreateBuffer(&desc, nullptr, &TestBuffers[j][i])))
					__debugbreak();
			}
		}

		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = 4096;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;

		if (FAILED(renderer->m_Device->CreateBuffer(&desc, nullptr, &TestLargeBuffer)))
			__debugbreak();

		ShaderConstantBuffer.Initialize(renderer->m_Device, D3D11_BIND_CONSTANT_BUFFER, ShaderConstantRingBufferSize, maxFrames);

		// Make sure first-time pointers are set up too
		OnNewFrame();
	}

	void Renderer::OnNewFrame()
	{
		if (FrameCompletedQueryPending[CurrentFrameIndex])
			__debugbreak();

		// Set a marker for when the GPU is done processing the previous frame
		GetGlobalsNonThreaded()->m_DeviceContext->End(FrameCompletedQueries[CurrentFrameIndex]);
		FrameCompletedQueryPending[CurrentFrameIndex] = true;

		DynamicBuffer.SwapFrame(CurrentFrameIndex);
		ShaderConstantBuffer.SwapFrame(CurrentFrameIndex);

		// "Pop" the query from 4 frames ago. This acts as a ring buffer.
		int prevQueryIndex = CurrentFrameIndex - 4;

		if (prevQueryIndex < 0)
			prevQueryIndex += 16;

		if (FrameCompletedQueryPending[prevQueryIndex])
		{
			BOOL data;
			HRESULT hr = GetGlobalsNonThreaded()->m_DeviceContext->GetData(FrameCompletedQueries[prevQueryIndex], &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH);

			// Those commands are REQUIRED to be complete by now - no exceptions
			if (FAILED(hr) || data != TRUE)
				__debugbreak();

			DynamicBuffer.FreeOldFrame(prevQueryIndex);
			ShaderConstantBuffer.FreeOldFrame(prevQueryIndex);

			FrameCompletedQueryPending[prevQueryIndex] = false;
		}

		FlushThreadedVars();
		CurrentFrameIndex++;

		if (CurrentFrameIndex >= 16)
			CurrentFrameIndex = 0;
	}

	ID3D11Buffer *Renderer::MapDynamicConstantBuffer(void **DataPointer, uint32_t *AllocationSize, uint32_t *AllocationOffset)
	{
		if (*AllocationSize <= 0)
			__debugbreak();

		// Size must be rounded up to nearest 256 bytes (D3D11.1 specification)
		uint32_t roundedAllocSize = (*AllocationSize + 256 - 1) & ~(256 - 1);

		ProfileCounterAdd("CB Bytes Wasted", (roundedAllocSize - *AllocationSize));

		*DataPointer = ShaderConstantBuffer.MapData(m_DeviceContext, roundedAllocSize, AllocationOffset, false);
		*AllocationSize = roundedAllocSize;

		return ShaderConstantBuffer.D3DBuffer;
	}

	const uint32_t ThresholdSize = 32;

	ID3D11Buffer *Renderer::MapConstantBuffer(void **DataPointer, uint32_t *AllocationSize, uint32_t *AllocationOffset, uint32_t Level)
	{
		ProfileCounterAdd("CB Bytes Requested", *AllocationSize);

		//
		// If the user lets us, try to use the global ring buffer instead of small temporary
		// allocations. It must be used on the immediate D3D context only. No MTR here.
		//
		if (*AllocationSize > ThresholdSize)
		{
			if (!MTRenderer::IsRenderingMultithreaded())
				return MapDynamicConstantBuffer(DataPointer, AllocationSize, AllocationOffset);
		}

		// Allocate from small constant buffer pool (TestBufferUsedBits is a TLS variable)
		if (*AllocationSize <= 0  || *AllocationSize > 4096)
			__debugbreak();

		if (Level >= ARRAYSIZE(TestBufferUsedBits))
			Level = ARRAYSIZE(TestBufferUsedBits) - 1;

		// Round to nearest 16, determine bit index, then loop until there's a free slot
		uint32_t roundedAllocSize = (*AllocationSize + 16 - 1) & ~(16 - 1);
		ID3D11Buffer *buffer = nullptr;

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

				// Move to the next largest buffer size
				bitIndex += 1;
				roundedAllocSize += 16;
			}
		}
		else
		{
			// Last-ditch effort to find a large valid buffer
			roundedAllocSize = 4096;
			buffer = TestLargeBuffer;
		}

		ProfileCounterAdd("CB Bytes Wasted", (roundedAllocSize - *AllocationSize));

		if (!buffer)
			__debugbreak();

		D3D11_MAPPED_SUBRESOURCE map;
		if (FAILED(m_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
			__debugbreak();

		*DataPointer = map.pData;
		*AllocationSize = map.RowPitch;

		if (AllocationOffset)
			*AllocationOffset = 0;

		return buffer;
	}

	void Renderer::UnmapDynamicConstantBuffer()
	{
		if (!MTRenderer::IsRenderingMultithreaded())
			ShaderConstantBuffer.UnmapData(m_DeviceContext);

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
			m_DynamicBuffers[0] = DynamicBuffer.D3DBuffer;
			m_CurrentDynamicBufferIndex = 0;
			m_FrameDataUsedSize = AllocationSize;

			return DynamicBuffer.MapData(m_DeviceContext, AllocationSize, AllocationOffset, true);
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

		if (logIndex < 7 || logIndex >= (11 + 7))
			__debugbreak();

		// Adjust base index - buffers start at 256 (2^7) bytes
		ID3D11Buffer *buffer = TempDynamicBuffers[logIndex - 7];

		D3D11_MAPPED_SUBRESOURCE map;
		if (FAILED(m_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
			__debugbreak();

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

	void Renderer::DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef)
	{
		if (*(DWORD *)&__zz0[40] != Mode || *(DWORD *)&__zz0[44] != StencilRef)
		{
			*(DWORD *)&__zz0[40] = Mode;
			*(DWORD *)&__zz0[44] = StencilRef;
			m_StateUpdateFlags |= 0x8;
		}
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

	// void BSGraphics::Renderer::SetTextureMode(unsigned int, enum  BSGraphics::TextureAddressMode, enum  BSGraphics::TextureFilterMode)
	void Renderer::SetTextureMode(uint32_t Index, uint32_t AddressMode, uint32_t FilterMode)
	{
		SetTextureAddressMode(Index, AddressMode);
		SetTextureFilterMode(Index, FilterMode);
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

	void Renderer::SetTexture(uint32_t Index, Texture *Resource)
	{
		SetShaderResource(Index, Resource ? Resource->m_D3DTexture : nullptr);
	}

	// Not a real function name
	void Renderer::SetShaderResource(uint32_t Index, ID3D11ShaderResourceView *Resource)
	{
		if (m_PSResources[Index] != Resource)
		{
			m_PSResources[Index] = Resource;
			m_PSResourceModifiedBits |= 1 << Index;
		}
	}

	ConstantGroup<BSVertexShader> Renderer::GetShaderConstantGroup(BSVertexShader *Shader, ConstantGroupLevel Level)
	{
		ConstantGroup<BSVertexShader> temp;
		temp.m_Unified = false;
		temp.m_Shader = Shader;
		temp.m_Buffer = Shader->m_ConstantGroups[Level].m_Buffer;
		memset(&temp.m_Map, 0, sizeof(temp.m_Map));

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

			temp.m_Buffer = MapConstantBuffer(&temp.m_Map.pData, &desc.ByteWidth, &temp.m_UnifiedByteOffset, Level);
			temp.m_Map.DepthPitch = desc.ByteWidth;
			temp.m_Map.RowPitch = desc.ByteWidth;
			temp.m_Unified = (temp.m_Buffer == ShaderConstantBuffer.D3DBuffer) ? true : false;
		}
		else
		{
			temp.m_Map.pData = Shader->m_ConstantGroups[Level].m_Data;
		}

		// BUGFIX: DirectX expects you to overwrite the entire buffer. **SKYRIM DOES NOT**, so I'm zeroing it now.
		memset(temp.m_Map.pData, 0, temp.m_Map.RowPitch);
		return temp;
	}

	ConstantGroup<BSPixelShader> Renderer::GetShaderConstantGroup(BSPixelShader *Shader, ConstantGroupLevel Level)
	{
		ConstantGroup<BSPixelShader> temp;
		temp.m_Unified = false;
		temp.m_Shader = Shader;
		temp.m_Buffer = Shader->m_ConstantGroups[Level].m_Buffer;
		memset(&temp.m_Map, 0, sizeof(temp.m_Map));

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

			temp.m_Buffer = MapConstantBuffer(&temp.m_Map.pData, &desc.ByteWidth, &temp.m_UnifiedByteOffset, Level);
			temp.m_Map.DepthPitch = desc.ByteWidth;
			temp.m_Map.RowPitch = desc.ByteWidth;
			temp.m_Unified = (temp.m_Buffer == ShaderConstantBuffer.D3DBuffer) ? true : false;
		}
		else
		{
			temp.m_Map.pData = Shader->m_ConstantGroups[Level].m_Data;
		}

		// BUGFIX: DirectX expects you to overwrite the entire buffer. **SKYRIM DOES NOT**, so I'm zeroing it now.
		memset(temp.m_Map.pData, 0, temp.m_Map.RowPitch);
		return temp;
	}

	void Renderer::FlushConstantGroupVSPS(const ConstantGroup<BSVertexShader> *VertexGroup, const ConstantGroup<BSPixelShader> *PixelGroup)
	{
		if (VertexGroup && VertexGroup->m_Buffer && !VertexGroup->m_Unified)
			m_DeviceContext->Unmap(VertexGroup->m_Buffer, 0);

		if (PixelGroup && PixelGroup->m_Buffer && !PixelGroup->m_Unified)
			m_DeviceContext->Unmap(PixelGroup->m_Buffer, 0);
	}

	void Renderer::ApplyConstantGroupVSPS(const ConstantGroup<BSVertexShader> *VertexGroup, const ConstantGroup<BSPixelShader> *PixelGroup, ConstantGroupLevel Level)
	{
		uint32_t index = 0;

		switch (Level)
		{
		case CONSTANT_GROUP_LEVEL_TECHNIQUE:index = 0; break;
		case CONSTANT_GROUP_LEVEL_MATERIAL:index = 1; break;
		case CONSTANT_GROUP_LEVEL_GEOMETRY:index = 2; break;
		default:__debugbreak(); break;
		}

		if (VertexGroup)
		{
			if (VertexGroup->m_Unified)
			{
				UINT offset = VertexGroup->m_UnifiedByteOffset / 16;
				UINT size = VertexGroup->m_Map.RowPitch / 16;
				((ID3D11DeviceContext1 *)m_DeviceContext)->VSSetConstantBuffers1(index, 1, &VertexGroup->m_Buffer, &offset, &size);
			}
			else
			{
				((ID3D11DeviceContext1 *)m_DeviceContext)->VSSetConstantBuffers1(index, 1, &VertexGroup->m_Buffer, nullptr, nullptr);
			}
		}

		if (PixelGroup)
		{
			if (PixelGroup->m_Unified)
			{
				UINT offset = PixelGroup->m_UnifiedByteOffset / 16;
				UINT size = PixelGroup->m_Map.RowPitch / 16;
				((ID3D11DeviceContext1 *)m_DeviceContext)->PSSetConstantBuffers1(index, 1, &PixelGroup->m_Buffer, &offset, &size);
			}
			else
			{
				((ID3D11DeviceContext1 *)m_DeviceContext)->PSSetConstantBuffers1(index, 1, &PixelGroup->m_Buffer, nullptr, nullptr);
			}
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
}