#pragma once

#include "BSShader/BSVertexShader.h"
#include "BSShader/BSPixelShader.h"

namespace BSGraphics
{
	enum ConstantGroupLevel
	{
		ConstantGroupLevel1 = 0,
		ConstantGroupLevel2 = 1,
		ConstantGroupLevel3 = 2,
	};

	class ConstantGroup
	{
	public:
		ConstantGroup()
		{
		}

		ConstantGroup(ID3D11Buffer *Buffer) : m_Buffer(Buffer)
		{
		}

		template<typename T, uint32_t ParamIndex, typename ShaderType>
		T& Param(ShaderType *Shader)
		{
			return *(T *)((uintptr_t)m_Map.pData + (sizeof(float) * Shader->m_ConstantOffsets[ParamIndex]));
		}

		D3D11_MAPPED_SUBRESOURCE m_Map;
		ID3D11Buffer *m_Buffer;
	};
}

namespace BSGraphics::Renderer
{
	void RasterStateSetCullMode(uint32_t CullMode);
	void AlphaBlendStateSetMode(uint32_t Mode);
	void AlphaBlendStateSetUnknown1(uint32_t Value);
	void AlphaBlendStateSetUnknown2(uint32_t Value);
	void DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef);
	void DepthStencilStateSetDepthMode(uint32_t Mode);
	void SetTextureFilterMode(uint32_t Index, uint32_t Mode);
	void SetTextureMode(uint32_t Index, uint32_t AddressMode, uint32_t FilterMode);
	void SetUseScrapConstantValue(bool UseStoredValue);
	void SetUseScrapConstantValue(bool UseStoredValue, float Value);

	void SetShaderResource(uint32_t Index, ID3D11ShaderResourceView *Resource);

	ConstantGroup GetShaderConstantGroup(BSVertexShader *Shader, ConstantGroupLevel Level);
	ConstantGroup GetShaderConstantGroup(BSPixelShader *Shader, ConstantGroupLevel Level);
	void FlushConstantGroup(ConstantGroup *Group);
	void ApplyConstantGroupVSPS(const ConstantGroup *VertexGroup, const ConstantGroup *PixelGroup, ConstantGroupLevel Level);

	void SetVertexShader(BSVertexShader *Shader);
	void SetPixelShader(BSPixelShader *Shader);
}