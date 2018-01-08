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

namespace BSGraphics::Renderer
{
	void RasterStateSetCullMode(uint32_t CullMode)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::RasterStateCullMode, CullMode))
			return;

		auto *renderer = GetThreadedGlobals();

		if (*(DWORD *)&renderer->__zz0[52] != CullMode)
		{
			*(DWORD *)&renderer->__zz0[52] = CullMode;
			renderer->dword_14304DEB0 |= 0x20;
		}
	}

	void RasterStateSetUnknown1(uint32_t Value)
	{
		auto *renderer = GetThreadedGlobals();

		if (*(DWORD *)&renderer->__zz0[56] != Value)
		{
			*(DWORD *)&renderer->__zz0[56] = Value;
			renderer->dword_14304DEB0 |= 0x40;
		}
	}

	void AlphaBlendStateSetMode(uint32_t Mode)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateMode, Mode))
			return;

		auto *renderer = GetThreadedGlobals();

		if (*(DWORD *)&renderer->__zz0[64] != Mode)
		{
			*(DWORD *)&renderer->__zz0[64] = Mode;
			renderer->dword_14304DEB0 |= 0x80;
		}
	}

	void AlphaBlendStateSetUnknown1(uint32_t Value)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateUnknown1, Value))
			return;

		auto *renderer = GetThreadedGlobals();

		if (*(DWORD *)&renderer->__zz0[68] != Value)
		{
			*(DWORD *)&renderer->__zz0[68] = Value;
			renderer->dword_14304DEB0 |= 0x80;
		}
	}

	void AlphaBlendStateSetUnknown2(uint32_t Value)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::AlphaBlendStateUnknown2, Value))
			return;

		auto *renderer = GetThreadedGlobals();

		if (*(DWORD *)&renderer->__zz0[72] != Value)
		{
			*(DWORD *)&renderer->__zz0[72] = Value;
			renderer->dword_14304DEB0 |= 0x80;
		}
	}

	void DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::DepthStencilStateStencilMode, Mode, StencilRef))
			return;

		auto *renderer = GetThreadedGlobals();

		if (*(DWORD *)&renderer->__zz0[40] != Mode || *(DWORD *)&renderer->__zz0[44] != StencilRef)
		{
			*(DWORD *)&renderer->__zz0[40] = Mode;
			*(DWORD *)&renderer->__zz0[44] = StencilRef;
			renderer->dword_14304DEB0 |= 0x8;
		}
	}

	void DepthStencilStateSetDepthMode(uint32_t Mode)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::DepthStencilStateDepthMode, Mode))
			return;

		auto *renderer = GetThreadedGlobals();

		if (*(DWORD *)&renderer->__zz0[32] != Mode)
		{
			*(DWORD *)&renderer->__zz0[32] = Mode;

			// Temp var to prevent duplicate state setting? Don't know where this gets set.
			if (*(DWORD *)&renderer->__zz0[36] != Mode)
				renderer->dword_14304DEB0 |= 0x4;
			else
				renderer->dword_14304DEB0 &= ~0x4;
		}
	}

	void SetTextureAddressMode(uint32_t Index, uint32_t Mode)
	{
		auto *renderer = GetThreadedGlobals();

		if (renderer->m_PSSamplerSetting1[Index] != Mode)
		{
			renderer->m_PSSamplerSetting1[Index] = Mode;
			renderer->m_PSSamplerModifiedBits |= 1 << Index;
		}
	}

	void SetTextureFilterMode(uint32_t Index, uint32_t Mode)
	{
		auto *renderer = GetThreadedGlobals();

		if (renderer->m_PSSamplerSetting2[Index] != Mode)
		{
			renderer->m_PSSamplerSetting2[Index] = Mode;
			renderer->m_PSSamplerModifiedBits |= 1 << Index;
		}
	}

	// void BSGraphics::Renderer::SetTextureMode(unsigned int, enum  BSGraphics::TextureAddressMode, enum  BSGraphics::TextureFilterMode)
	void SetTextureMode(uint32_t Index, uint32_t AddressMode, uint32_t FilterMode)
	{
		auto *renderer = GetThreadedGlobals();

		if (renderer->m_PSSamplerSetting1[Index] != AddressMode)
		{
			renderer->m_PSSamplerSetting1[Index] = AddressMode;
			renderer->m_PSSamplerModifiedBits |= 1 << Index;
		}

		if (renderer->m_PSSamplerSetting2[Index] != FilterMode)
		{
			renderer->m_PSSamplerSetting2[Index] = FilterMode;
			renderer->m_PSSamplerModifiedBits |= 1 << Index;
		}
	}

	void SetUseScrapConstantValue(bool UseStoredValue)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::UseScrapConstantValue_1, UseStoredValue))
			return;

		auto *renderer = GetThreadedGlobals();

		// When UseStoredValue is false, the constant buffer data is zeroed, but float_14304DF68 is saved
		if (renderer->__zz0[76] != UseStoredValue)
		{
			renderer->__zz0[76] = UseStoredValue;
			renderer->dword_14304DEB0 |= 0x100u;
		}
	}

	void SetUseScrapConstantValue(bool UseStoredValue, float Value)
	{
		if (InsertRenderCommand<SetStateRenderCommand>(SetStateRenderCommand::UseScrapConstantValue_2, UseStoredValue, *(uint32_t *)&Value))
			return;

		auto *renderer = GetThreadedGlobals();

		if (renderer->__zz0[76] != UseStoredValue)
		{
			renderer->__zz0[76] = UseStoredValue;
			renderer->dword_14304DEB0 |= 0x100u;
		}

		if (renderer->float_14304DF68 != Value)
		{
			renderer->float_14304DF68 = Value;
			renderer->dword_14304DEB0 |= 0x200u;
		}
	}

	// Not a real function name
	void SetShaderResource(uint32_t Index, ID3D11ShaderResourceView *Resource)
	{
		auto *renderer = GetThreadedGlobals();

		if (renderer->m_PSResources[Index] != Resource)
		{
			renderer->m_PSResources[Index] = Resource;
			renderer->m_PSResourceModifiedBits |= 1 << Index;
		}
	}

	ConstantGroup<BSVertexShader> GetShaderConstantGroup(BSVertexShader *Shader, ConstantGroupLevel Level)
	{
		ConstantGroup<BSVertexShader> temp;
		temp.m_Shader = Shader;
		temp.m_Buffer = Shader->m_ConstantGroups[Level].m_Buffer;
		memset(&temp.m_Map, 0, sizeof(temp.m_Map));

		if (temp.m_Buffer)
		{
			auto *renderer = GetThreadedGlobals();
			HRESULT hr = renderer->m_DeviceContext->Map(temp.m_Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &temp.m_Map);

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

	ConstantGroup<BSPixelShader> GetShaderConstantGroup(BSPixelShader *Shader, ConstantGroupLevel Level)
	{
		ConstantGroup<BSPixelShader> temp;
		temp.m_Shader = Shader;
		temp.m_Buffer = Shader->m_ConstantGroups[Level].m_Buffer;
		memset(&temp.m_Map, 0, sizeof(temp.m_Map));

		if (temp.m_Buffer)
		{
			auto *renderer = GetThreadedGlobals();
			HRESULT hr = renderer->m_DeviceContext->Map(temp.m_Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &temp.m_Map);

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

	void FlushConstantGroupVSPS(const ConstantGroup<BSVertexShader> *VertexGroup, const ConstantGroup<BSPixelShader> *PixelGroup)
	{
		if (VertexGroup && VertexGroup->m_Buffer)
			GetThreadedGlobals()->m_DeviceContext->Unmap(VertexGroup->m_Buffer, 0);

		if (PixelGroup && PixelGroup->m_Buffer)
			GetThreadedGlobals()->m_DeviceContext->Unmap(PixelGroup->m_Buffer, 0);
	}

	void ApplyConstantGroupVSPS(const ConstantGroup<BSVertexShader> *VertexGroup, const ConstantGroup<BSPixelShader> *PixelGroup, ConstantGroupLevel Level)
	{
		auto *renderer = GetThreadedGlobals();
		uint32_t index = 0;

		switch (Level)
		{
		case CONSTANT_GROUP_LEVEL_TECHNIQUE:index = 0; break;
		case CONSTANT_GROUP_LEVEL_MATERIAL:index = 1; break;
		case CONSTANT_GROUP_LEVEL_GEOMETRY:index = 2; break;
		default:__debugbreak(); break;
		}

		if (VertexGroup)
			renderer->m_DeviceContext->VSSetConstantBuffers(index, 1, &VertexGroup->m_Buffer);

		if (PixelGroup)
			renderer->m_DeviceContext->PSSetConstantBuffers(index, 1, &PixelGroup->m_Buffer);
	}

	void SetVertexShader(BSVertexShader *Shader)
	{
		auto *renderer = GetThreadedGlobals();

		// The input layout (IASetInputLayout) needs to be created and updated
		renderer->m_CurrentVertexShader = Shader;
		renderer->dword_14304DEB0 |= 0x400;

		renderer->m_DeviceContext->VSSetShader(Shader->m_Shader, nullptr, 0);
	}

	void SetPixelShader(BSPixelShader *Shader)
	{
		auto *renderer = GetThreadedGlobals();

		renderer->m_CurrentPixelShader = Shader;
		renderer->m_DeviceContext->PSSetShader(Shader->m_Shader, nullptr, 0);
	}
}