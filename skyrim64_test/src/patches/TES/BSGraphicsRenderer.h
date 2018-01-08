#pragma once

#include <DirectXMath.h>
#include "NiMain/NiColor.h"
#include "BSShader/BSVertexShader.h"
#include "BSShader/BSPixelShader.h"

namespace BSGraphics
{
	enum ConstantGroupLevel
	{
		CONSTANT_GROUP_LEVEL_TECHNIQUE = 0x0,
		CONSTANT_GROUP_LEVEL_MATERIAL = 0x1,
		CONSTANT_GROUP_LEVEL_GEOMETRY = 0x2,
		CONSTANT_GROUP_LEVEL_COUNT = 0x3,

		CONSTANT_GROUP_LEVEL_INSTANCE = 0x8,
		CONSTANT_GROUP_LEVEL_PREVIOUS_BONES = 0x9,
		CONSTANT_GROUP_LEVEL_BONES = 0xA,
		//CONSTANT_GROUP_LEVEL_SUB_INDEX = 0xB,				Unconfirmed (FO4)
		//CONSTANT_GROUP_LEVEL_FACE_CUSTOMIZATION = 0xC,	Unconfirmed (FO4)
	};

	template<typename T>
	class ConstantGroup
	{
		/*
		friend ConstantGroup<BSVertexShader> BSGraphics::Renderer::GetShaderConstantGroup(BSVertexShader *Shader, ConstantGroupLevel Level);
		friend ConstantGroup<BSPixelShader> BSGraphics::Renderer::GetShaderConstantGroup(BSPixelShader *Shader, ConstantGroupLevel Level);
		friend void BSGraphics::Renderer::FlushConstantGroup(ConstantGroup<BSVertexShader> *Group);
		friend void BSGraphics::Renderer::FlushConstantGroup(ConstantGroup<BSPixelShader> *Group);
		friend void BSGraphics::Renderer::ApplyConstantGroupVSPS(const ConstantGroup<BSVertexShader> *VertexGroup, const ConstantGroup<BSPixelShader> *PixelGroup, ConstantGroupLevel Level);
		*/
	public:
		T *m_Shader;
		ID3D11Buffer *m_Buffer;
		D3D11_MAPPED_SUBRESOURCE m_Map;

	public:
		template<typename U, uint32_t ParamIndex>
		U& ParamVS() const
		{
			static_assert(std::is_same<T, BSVertexShader>::value, "ParamVS() requires ConstantGroup<BSVertexShader>");
			static_assert(ParamIndex < ARRAYSIZE(T::m_ConstantOffsets));

			uintptr_t data		= (uintptr_t)m_Map.pData;
			uintptr_t offset	= m_Shader->m_ConstantOffsets[ParamIndex] * sizeof(float);

			return *(U *)(data + offset);
		}

		template<typename U, uint32_t ParamIndex>
		U& ParamPS() const
		{
			static_assert(std::is_same<T, BSPixelShader>::value, "ParamPS() requires ConstantGroup<BSPixelShader>");
			static_assert(ParamIndex < ARRAYSIZE(T::m_ConstantOffsets));

			uintptr_t data = (uintptr_t)m_Map.pData;
			uintptr_t offset = m_Shader->m_ConstantOffsets[ParamIndex] * sizeof(float);

			return *(U *)(data + offset);
		}

		void *RawData() const
		{
			return m_Map.pData;
		}
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

	ConstantGroup<BSVertexShader> GetShaderConstantGroup(BSVertexShader *Shader, ConstantGroupLevel Level);
	ConstantGroup<BSPixelShader> GetShaderConstantGroup(BSPixelShader *Shader, ConstantGroupLevel Level);
	void FlushConstantGroupVSPS(const ConstantGroup<BSVertexShader> *VertexGroup, const ConstantGroup<BSPixelShader> *PixelGroup);
	void ApplyConstantGroupVSPS(const ConstantGroup<BSVertexShader> *VertexGroup, const ConstantGroup<BSPixelShader> *PixelGroup, ConstantGroupLevel Level);

	void SetVertexShader(BSVertexShader *Shader);
	void SetPixelShader(BSPixelShader *Shader);
}