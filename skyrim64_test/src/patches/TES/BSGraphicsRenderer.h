#pragma once

#include <DirectXMath.h>
#include "NiMain/NiColor.h"
#include "BSShader/BSVertexShader.h"
#include "BSShader/BSPixelShader.h"

namespace BSGraphics
{
	enum ConstantGroupLevel
	{
		ConstantGroupLevel1 = 0,
		ConstantGroupLevel2 = 1,
		ConstantGroupLevel3 = 2,

		// Grass instance data @ constant buffer 8
		// Bone matrix setting @ constant buffer 9
		// Bone matrix setting @ constant buffer 10
	};

	class ConstantGroup
	{
	public:
		ConstantGroup()
		{
			memset(&m_Map, 0, sizeof(m_Map));
		}

		ConstantGroup(ID3D11Buffer *Buffer) : m_Buffer(Buffer)
		{
			memset(&m_Map, 0, sizeof(m_Map));
		}

		template<typename T, uint32_t ParamIndex, typename ShaderType>
		T& Param(ShaderType *Shader)
		{
			const uintptr_t data	= (uintptr_t)m_Map.pData;
			const uintptr_t offset	= Shader->m_ConstantOffsets[ParamIndex] * sizeof(float);

			return *(T *)(data + offset);
		}

		D3D11_MAPPED_SUBRESOURCE m_Map;
		ID3D11Buffer *m_Buffer;
	};
}

namespace BSGraphics::Utility
{
	void CopyNiColorAToFloat(float *Floats, const NiColorA& Color);
	void CopyNiColorAToFloat(DirectX::XMVECTOR *Floats, const NiColorA& Color);
}

namespace BSGraphics::Renderer
{
	void RasterStateSetCullMode(uint32_t CullMode);
	void RasterStateSetUnknown1(uint32_t Value);

	void AlphaBlendStateSetMode(uint32_t Mode);
	void AlphaBlendStateSetUnknown1(uint32_t Value);
	void AlphaBlendStateSetUnknown2(uint32_t Value);
	void DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef);
	void DepthStencilStateSetDepthMode(uint32_t Mode);

	void SetTextureAddressMode(uint32_t Index, uint32_t Mode);
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