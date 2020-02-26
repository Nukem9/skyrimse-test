#pragma once

#include <functional>
#include <DirectXMath.h>
#include "NiMain/NiColor.h"
#include "NiMain/NiTransform.h"
#include "BSGraphicsTypes.h"
#include "NiMain/NiSourceTexture.h"

class BSTriShape;
class BSDynamicTriShape;

namespace BSGraphics::Utility
{
	void CopyNiColorAToFloat(float *Floats, const NiColorA& Color);
	void CopyNiColorAToFloat(DirectX::XMVECTOR *Floats, const NiColorA& Color);
	void PackDynamicParticleData(uint32_t ParticleCount, class NiParticles *Particles, void *Buffer);
}

namespace BSGraphics
{
	void BeginEvent(wchar_t *Marker);
	void EndEvent();

	class Renderer
	{
	public:
		static Renderer *GetGlobals();
		static Renderer *GetGlobalsNonThreaded();

		static void Initialize(ID3D11Device2 *Device);
		static void OnNewFrame();

		static void FlushThreadedVars();

		//
		// Debug
		//
		void BeginEvent(wchar_t *Marker) const;
		void EndEvent() const;

		//
		// Drawing
		//
		void DrawLineShape(LineShape *GraphicsLineShape, uint32_t StartIndex, uint32_t Count);

		void DrawTriShape(TriShape *GraphicsTriShape, uint32_t StartIndex, uint32_t Count);

		DynamicTriShape *GetParticlesDynamicTriShape();
		void *MapDynamicTriShapeDynamicData(BSDynamicTriShape *Shape, DynamicTriShape *ShapeData, DynamicTriShapeDrawData *DrawData, uint32_t VertexSize);
		void UnmapDynamicTriShapeDynamicData(DynamicTriShape *Shape, DynamicTriShapeDrawData *DrawData);
		void DrawDynamicTriShape(DynamicTriShape *Shape, DynamicTriShapeDrawData *DrawData, uint32_t IndexStartOffset, uint32_t TriangleCount);
		void DrawDynamicTriShape(UnknownStruct *Params, DynamicTriShapeDrawData *DrawData, uint32_t IndexStartOffset, uint32_t TriangleCount, uint32_t VertexBufferOffset);

		void DrawParticleShaderTriShape(const void *DynamicData, uint32_t Count);

		//
		// State management
		//
		static void SetDirtyStates(bool IsComputeShader);
		static void FlushD3DResources();

		void DepthStencilStateSetDepthMode(DepthStencilDepthMode Mode);
		DepthStencilDepthMode DepthStencilStateGetDepthMode() const;
		void DepthStencilStateSetStencilMode(uint32_t Mode, uint32_t StencilRef);

		void RasterStateSetCullMode(uint32_t CullMode);
		void RasterStateSetDepthBias(uint32_t Value);

		void AlphaBlendStateSetMode(uint32_t Mode);
		void AlphaBlendStateSetAlphaToCoverage(uint32_t Value);
		void AlphaBlendStateSetWriteMode(uint32_t Value);
		uint32_t AlphaBlendStateGetWriteMode() const;

		void SetUseAlphaTestRef(bool UseStoredValue);
		void SetAlphaTestRef(float Value);

		void SetVertexDescription(uint64_t VertexDesc);
		void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology);

		void SetVertexShader(VertexShader *Shader);
		void SetPixelShader(PixelShader *Shader);
		void SetHullShader(HullShader *Shader);
		void SetDomainShader(DomainShader *Shader);

		void SetTexture(uint32_t Index, const NiSourceTexture *Texture);
		void SetTexture(uint32_t Index, const Texture *Resource);
		void SetTextureMode(uint32_t Index, uint32_t AddressMode, uint32_t FilterMode);
		void SetTextureAddressMode(uint32_t Index, uint32_t Mode);
		void SetTextureFilterMode(uint32_t Index, uint32_t Mode);

		void SetShaderResource(uint32_t Index, ID3D11ShaderResourceView *Resource);

		//
		// Buffer management
		//
		ID3D11Buffer *MapConstantBuffer(void **DataPointer, uint32_t *AllocationSize, uint32_t *AllocationOffset, uint32_t Level);
		void UnmapDynamicConstantBuffer();
		void *MapDynamicBuffer(uint32_t AllocationSize, uint32_t *AllocationOffset);

		//
		// Shader creation
		//
		void ValidateShaderReplacement(ID3D11PixelShader *Original, ID3D11PixelShader *Replacement);
		void ValidateShaderReplacement(ID3D11VertexShader *Original, ID3D11VertexShader *Replacement);
		void ValidateShaderReplacement(ID3D11ComputeShader *Original, ID3D11ComputeShader *Replacement);
		void ValidateShaderReplacement(void *Original, void *Replacement, const GUID& Guid);
		void RegisterShaderBytecode(void *Shader, const void *Bytecode, size_t BytecodeLength);
		const std::pair<void *, size_t>& GetShaderBytecode(void *Shader);

		ID3DBlob *CompileShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, const char *ProgramType);
		VertexShader *CompileVertexShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetConstant);
		PixelShader *CompilePixelShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetSampler, std::function<const char *(int Index)> GetConstant);
		HullShader *CompileHullShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines);
		DomainShader *CompileDomainShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines);

		//
		// Shader constant buffers
		//
		CustomConstantGroup GetShaderConstantGroup(uint32_t Size, ConstantGroupLevel Level);
		VertexCGroup GetShaderConstantGroup(VertexShader *Shader, ConstantGroupLevel Level);
		PixelCGroup GetShaderConstantGroup(PixelShader *Shader, ConstantGroupLevel Level);
		void FlushConstantGroup(CustomConstantGroup *Group);
		void FlushConstantGroupVSPS(ConstantGroup<VertexShader> *VertexGroup, ConstantGroup<PixelShader> *PixelGroup);
		void ApplyConstantGroupVS(const CustomConstantGroup *Group, ConstantGroupLevel Level);
		void ApplyConstantGroupPS(const CustomConstantGroup *Group, ConstantGroupLevel Level);
		void ApplyConstantGroupVSPS(const ConstantGroup<VertexShader> *VertexGroup, const ConstantGroup<PixelShader> *PixelGroup, ConstantGroupLevel Level);

		//
		// Geometry
		//
		void IncRef(TriShape *Shape);
		void DecRef(TriShape *Shape);
		void IncRef(DynamicTriShape *Shape);
		void DecRef(DynamicTriShape *Shape);

		struct
		{
			float			m_PreviousClearColor[4];			// Render target clear color

			void			*qword_14304BF00;					// Unknown class pointer

			ID3D11Device	*m_Device;
			HWND			m_Window;

			//
			// These are pools for efficient data uploads to the GPU. Each frame can use any buffer as long as there
			// is sufficient space. If there's no space left, delay execution until m_CommandListEndEvents[] says a buffer
			// is no longer in use.
			//
			ID3D11Buffer		*m_DynamicBuffers[3];			// DYNAMIC (VERTEX | INDEX) CPU_ACCESS_WRITE
			uint32_t			m_CurrentDynamicBufferIndex;

			uint32_t			m_FrameDataUsedSize;			// Used in relation with m_CommandListEndEvents[]
			ID3D11Buffer		*m_UnknownIndexBuffer;			// DEFAULT INDEX CPU_ACCESS_NONE
			ID3D11Buffer		*m_UnknownVertexBuffer;			// DEFAULT VERTEX CPU_ACCESS_NONE
			ID3D11InputLayout	*m_UnknownInputLayout;
			ID3D11InputLayout	*m_UnknownInputLayout2;
			uint32_t			m_UnknownCounter;				// 0 to 63
			uint32_t			m_UnknownCounter2;				// No limits
			void				*m_UnknownStaticBuffer[64];
			uint32_t			m_UnknownCounter3;				// 0 to 5
			bool				m_EventQueryFinished[3];
			ID3D11Query			*m_CommandListEndEvents[3];		// D3D11_QUERY_EVENT (Waits for a series of commands to finish execution)

			float m_DepthBiasFactors[3][4];

			ID3D11DepthStencilState *m_DepthStates[6][40];		// OMSetDepthStencilState
			ID3D11RasterizerState *m_RasterStates[2][3][12][2];	// RSSetState
			ID3D11BlendState *m_BlendStates[7][2][13][2];		// OMSetBlendState
			ID3D11SamplerState *m_SamplerStates[6][5];			// Samplers[AddressMode][FilterMode] (Used for PS and CS)

			//
			// Vertex/Pixel shader constant buffers. Set during load-time (CreateShaderBundle).
			//
			uint32_t		m_NextConstantBufferIndex;
			ID3D11Buffer	*m_ConstantBuffers1[4];				// Sizes: 3840 bytes
			ID3D11Buffer	*m_AlphaTestRefCB;					// CONSTANT_GROUP_LEVEL_ALPHA_TEST_REF (Index 11) - 16 bytes
			ID3D11Buffer	*m_ConstantBuffers2[20];			// Sizes: 0, 16, 32, 48, ... 304 bytes
			ID3D11Buffer	*m_ConstantBuffers3[10];			// Sizes: 0, 16, 32, 48, ... 144 bytes
			ID3D11Buffer	*m_ConstantBuffers4[28];			// Sizes: 0, 16, 32, 48, ... 432 bytes
			ID3D11Buffer	*m_ConstantBuffers5[20];			// Sizes: 0, 16, 32, 48, ... 304 bytes
			ID3D11Buffer	*m_ConstantBuffers6[20];			// Sizes: 0, 16, 32, 48, ... 304 bytes
			ID3D11Buffer	*m_ConstantBuffers7[40];			// Sizes: 0, 16, 32, 48, ... 624 bytes
			ID3D11Buffer	*m_TempConstantBuffer2;				// 576 bytes
			ID3D11Buffer	*m_TempConstantBuffer3;				// CONSTANT_GROUP_LEVEL_PERFRAME (Index 12) - 720 bytes
			ID3D11Buffer	*m_TempConstantBuffer4;				// 16 bytes

			IDXGIOutput *m_DXGIAdapterOutput;
			ID3D11DeviceContext2 *m_DeviceContext;

			void *m_FrameDurationStringHandle;					// "Frame Duration" but stored in their global string pool

			//
			// This is a separate structure with a global instance: BSGraphics::RendererShadowState
			//
			uint32_t m_StateUpdateFlags;						// Flags +0x0  0xFFFFFFFF; global state updates
			uint32_t m_PSResourceModifiedBits;					// Flags +0x4  0xFFFF
			uint32_t m_PSSamplerModifiedBits;					// Flags +0x8  0xFFFF
			uint32_t m_CSResourceModifiedBits;					// Flags +0xC  0xFFFF
			uint32_t m_CSSamplerModifiedBits;					// Flags +0x10 0xFFFF
			uint32_t m_CSUAVModifiedBits;						// Flags +0x14 0xFF

			uint32_t m_RenderTargets[8];						// enum <unnamed>: RENDER_TARGET_NONE...RENDER_TARGET_CUBEMAP_REFLECTIONS
			uint32_t m_DepthStencil;							// Index
			uint32_t m_DepthStencilSlice;						// Index
			uint32_t m_CubeMapRenderTarget;						// Index
			uint32_t m_CubeMapRenderTargetView;					// Index

			SetRenderTargetMode m_SetRenderTargetMode[8];
			SetRenderTargetMode m_SetDepthStencilMode;
			SetRenderTargetMode m_SetCubeMapRenderTargetMode;

			D3D11_VIEWPORT m_ViewPort;

			DepthStencilDepthMode m_DepthStencilDepthMode;
			uint32_t m_DepthStencilUnknown;
			uint32_t m_DepthStencilStencilMode;
			uint32_t m_StencilRef;

			uint32_t m_RasterStateFillMode;
			uint32_t m_RasterStateCullMode;
			uint32_t m_RasterStateDepthBiasMode;
			uint32_t m_RasterStateScissorMode;

			uint32_t m_AlphaBlendMode;
			uint32_t m_AlphaBlendAlphaToCoverage;
			uint32_t m_AlphaBlendWriteMode;

			bool m_AlphaTestEnabled;
			float m_AlphaTestRef;

			uint32_t m_PSTextureAddressMode[16];
			uint32_t m_PSTextureFilterMode[16];
			ID3D11ShaderResourceView *m_PSTexture[16];

			uint32_t m_CSTextureAddressMode[16];
			uint32_t m_CSTextureFilterMode[16];

			ID3D11ShaderResourceView *m_CSTexture[16];
			uint32_t m_CSTextureMinLodMode[16];
			ID3D11UnorderedAccessView *m_CSUAV[8];

			union
			{
				struct
				{
					uint64_t m_VertexDesc;
					VertexShader *m_CurrentVertexShader;
					PixelShader *m_CurrentPixelShader;
					D3D11_PRIMITIVE_TOPOLOGY m_Topology;
					NiPoint3 m_PosAdjust;
					NiPoint3 m_PreviousPosAdjust;
					char _zpad3[0x3C];
					DirectX::XMMATRIX m_ViewMat;
					DirectX::XMMATRIX m_ProjMat;
					DirectX::XMMATRIX m_ViewProjMat;
					char _a_pad4[0x40];
					DirectX::XMMATRIX m_ViewProjMatrixUnjittered;
					DirectX::XMMATRIX m_PreviousViewProjMatrixUnjittered;
					DirectX::XMMATRIX m_ProjMatrixUnjittered;
					char _a_zz2[0x70];
				};

				char __zz2[0x2A0];
			};
		};
	};

#define CHECK_OFFSET(member, actualAddr) static_assert(offsetof(Renderer, member) == (actualAddr - 0x14304BEF0), "")
	static_assert(sizeof(Renderer) == 0x25A0, "");
	CHECK_OFFSET(m_PreviousClearColor, 0x14304BEF0);
	CHECK_OFFSET(qword_14304BF00, 0x14304BF00);
	CHECK_OFFSET(m_Device, 0x14304BF08);
	CHECK_OFFSET(m_Window, 0x14304BF10);
	CHECK_OFFSET(m_DynamicBuffers, 0x14304BF18);
	CHECK_OFFSET(m_CurrentDynamicBufferIndex, 0x14304BF30);
	CHECK_OFFSET(m_FrameDataUsedSize, 0x14304BF34);
	CHECK_OFFSET(m_UnknownIndexBuffer, 0x14304BF38);
	CHECK_OFFSET(m_UnknownVertexBuffer, 0x14304BF40);
	CHECK_OFFSET(m_UnknownInputLayout, 0x14304BF48);
	CHECK_OFFSET(m_UnknownInputLayout2, 0x14304BF50);
	CHECK_OFFSET(m_UnknownCounter, 0x14304BF58);
	CHECK_OFFSET(m_UnknownCounter2, 0x14304BF5C);
	CHECK_OFFSET(m_UnknownStaticBuffer, 0x14304BF60);
	CHECK_OFFSET(m_UnknownCounter3, 0x14304C160);
	CHECK_OFFSET(m_EventQueryFinished, 0x14304C164);
	CHECK_OFFSET(m_CommandListEndEvents, 0x14304C168);
	CHECK_OFFSET(m_DepthBiasFactors, 0x14304C180);
	CHECK_OFFSET(m_DepthStates, 0x14304C1B0);
	CHECK_OFFSET(m_RasterStates, 0x14304C930);
	CHECK_OFFSET(m_BlendStates, 0x14304CDB0);
	CHECK_OFFSET(m_SamplerStates, 0x14304D910);
	CHECK_OFFSET(m_NextConstantBufferIndex, 0x14304DA00);
	CHECK_OFFSET(m_ConstantBuffers1, 0x14304DA08);
	CHECK_OFFSET(m_AlphaTestRefCB, 0x14304DA28);
	CHECK_OFFSET(m_ConstantBuffers2, 0x14304DA30);
	CHECK_OFFSET(m_ConstantBuffers3, 0x14304DAD0);
	CHECK_OFFSET(m_ConstantBuffers4, 0x14304DB20);
	CHECK_OFFSET(m_ConstantBuffers5, 0x14304DC00);
	CHECK_OFFSET(m_ConstantBuffers6, 0x14304DCA0);
	CHECK_OFFSET(m_ConstantBuffers7, 0x14304DD40);
	CHECK_OFFSET(m_TempConstantBuffer2, 0x14304DE80);
	CHECK_OFFSET(m_TempConstantBuffer3, 0x14304DE88);
	CHECK_OFFSET(m_TempConstantBuffer4, 0x14304DE90);
	CHECK_OFFSET(m_DXGIAdapterOutput, 0x14304DE98);
	CHECK_OFFSET(m_DeviceContext, 0x14304DEA0);
	CHECK_OFFSET(m_FrameDurationStringHandle, 0x14304DEA8);
	CHECK_OFFSET(m_StateUpdateFlags, 0x14304DEB0);
	CHECK_OFFSET(m_PSResourceModifiedBits, 0x14304DEB4);
	CHECK_OFFSET(m_PSSamplerModifiedBits, 0x14304DEB8);
	CHECK_OFFSET(m_CSResourceModifiedBits, 0x14304DEBC);
	CHECK_OFFSET(m_CSSamplerModifiedBits, 0x14304DEC0);
	CHECK_OFFSET(m_CSUAVModifiedBits, 0x14304DEC4);
	CHECK_OFFSET(m_RenderTargets, 0x14304DEC8);
	CHECK_OFFSET(m_DepthStencil, 0x14304DEE8);
	CHECK_OFFSET(m_DepthStencilSlice, 0x14304DEEC);
	CHECK_OFFSET(m_SetRenderTargetMode, 0x14304DEF8);
	CHECK_OFFSET(m_AlphaTestRef, 0x14304DF68);
	CHECK_OFFSET(m_PSTextureAddressMode, 0x14304DF6C);
	CHECK_OFFSET(m_PSTextureFilterMode, 0x14304DFAC);
	CHECK_OFFSET(m_PSTexture, 0x14304DFF0);
	CHECK_OFFSET(m_CSTextureAddressMode, 0x14304E070);
	CHECK_OFFSET(m_CSTextureFilterMode, 0x14304E0B0);
	CHECK_OFFSET(m_CSTexture, 0x14304E0F0);
	CHECK_OFFSET(m_CSUAV, 0x14304E1B0);
	CHECK_OFFSET(m_VertexDesc, 0x14304E1F0);
	CHECK_OFFSET(m_CurrentVertexShader, 0x14304E1F8);
	CHECK_OFFSET(m_CurrentPixelShader, 0x14304E200);
	CHECK_OFFSET(m_Topology, 0x14304E208);
	CHECK_OFFSET(m_PosAdjust, 0x14304E20C);
	CHECK_OFFSET(m_PreviousPosAdjust, 0x14304E218);
	CHECK_OFFSET(m_ViewMat, 0x14304E260);
	CHECK_OFFSET(m_ProjMat, 0x14304E2A0);
	CHECK_OFFSET(m_ViewProjMat, 0x14304E2E0);
	CHECK_OFFSET(m_ViewProjMatrixUnjittered, 0x14304E360);
	CHECK_OFFSET(m_PreviousViewProjMatrixUnjittered, 0x14304E3A0);
	CHECK_OFFSET(m_ProjMatrixUnjittered, 0x14304E3E0);
}