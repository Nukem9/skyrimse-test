#include "../rendering/common.h"
#include "BSGraphicsRenderer.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSVertexShader.h"
#include "BSShader/BSPixelShader.h"

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
	thread_local BSVertexShader *TLS_CurrentVertexShader;
	thread_local BSPixelShader *TLS_CurrentPixelShader;

	Renderer *Renderer::GetGlobals()
	{
		return (Renderer *)HACK_GetThreadedGlobals();
	}

	Renderer *Renderer::GetGlobalsNonThreaded()
	{
		return (Renderer *)HACK_GetMainGlobals();
	}

	void Renderer::FlushThreadedVars()
	{
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
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::RasterStateCullMode, CullMode))
			return;

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
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateMode, Mode))
			return;

		if (*(DWORD *)&__zz0[64] != Mode)
		{
			*(DWORD *)&__zz0[64] = Mode;
			m_StateUpdateFlags |= 0x80;
		}
	}

	void Renderer::AlphaBlendStateSetUnknown1(uint32_t Value)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateUnknown1, Value))
			return;

		if (*(DWORD *)&__zz0[68] != Value)
		{
			*(DWORD *)&__zz0[68] = Value;
			m_StateUpdateFlags |= 0x80;
		}
	}

	void Renderer::AlphaBlendStateSetUnknown2(uint32_t Value)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateUnknown2, Value))
			return;

		if (*(DWORD *)&__zz0[72] != Value)
		{
			*(DWORD *)&__zz0[72] = Value;
			m_StateUpdateFlags |= 0x80;
		}
	}

	void Renderer::DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::DepthStencilStateStencilMode, Mode, StencilRef))
			return;

		if (*(DWORD *)&__zz0[40] != Mode || *(DWORD *)&__zz0[44] != StencilRef)
		{
			*(DWORD *)&__zz0[40] = Mode;
			*(DWORD *)&__zz0[44] = StencilRef;
			m_StateUpdateFlags |= 0x8;
		}
	}

	void Renderer::DepthStencilStateSetDepthMode(uint32_t Mode)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::DepthStencilStateDepthMode, Mode))
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
		if (m_PSSamplerAddressMode[Index] != AddressMode)
		{
			m_PSSamplerAddressMode[Index] = AddressMode;
			m_PSSamplerModifiedBits |= 1 << Index;
		}

		if (m_PSSamplerFilterMode[Index] != FilterMode)
		{
			m_PSSamplerFilterMode[Index] = FilterMode;
			m_PSSamplerModifiedBits |= 1 << Index;
		}
	}

	void Renderer::SetUseScrapConstantValue(bool UseStoredValue)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::UseScrapConstantValue_1, UseStoredValue))
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
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::UseScrapConstantValue_2, *(uint32_t *)&Value))
			return;

		if (m_ScrapConstantValue != Value)
		{
			m_ScrapConstantValue = Value;
			m_StateUpdateFlags |= 0x200u;
		}
	}

	void Renderer::SetTexture(uint32_t Index, Texture *Resource)
	{
		ID3D11ShaderResourceView *ptr = Resource ? Resource->m_D3DTexture : nullptr;

		if (m_PSResources[Index] != ptr)
		{
			m_PSResources[Index] = ptr;
			m_PSResourceModifiedBits |= 1 << Index;
		}
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
		temp.m_Shader = Shader;
		temp.m_Buffer = Shader->m_ConstantGroups[Level].m_Buffer;
		memset(&temp.m_Map, 0, sizeof(temp.m_Map));

		if (temp.m_Buffer)
		{
			HRESULT hr = m_DeviceContext->Map(temp.m_Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &temp.m_Map);

			if (FAILED(hr))
				__debugbreak();
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
		temp.m_Shader = Shader;
		temp.m_Buffer = Shader->m_ConstantGroups[Level].m_Buffer;
		memset(&temp.m_Map, 0, sizeof(temp.m_Map));

		if (temp.m_Buffer)
		{
			HRESULT hr = m_DeviceContext->Map(temp.m_Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &temp.m_Map);

			if (FAILED(hr))
				__debugbreak();
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
		if (VertexGroup && VertexGroup->m_Buffer)
			m_DeviceContext->Unmap(VertexGroup->m_Buffer, 0);

		if (PixelGroup && PixelGroup->m_Buffer)
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
			m_DeviceContext->VSSetConstantBuffers(index, 1, &VertexGroup->m_Buffer);

		if (PixelGroup)
			m_DeviceContext->PSSetConstantBuffers(index, 1, &PixelGroup->m_Buffer);
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