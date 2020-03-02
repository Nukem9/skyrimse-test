#pragma once

#include <wrl/client.h>
#include <functional>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "../BSGraphicsTypes.h"
#include "../BSGraphicsState.h"
#include "../BSShader/BSShaderRenderTargets.h"

namespace BSGraphics
{
	using Microsoft::WRL::ComPtr;

	// Not an actual class
	class HACK_Globals
	{
	public:
		float m_PreviousClearColor[4];			// Render target clear color
		void *qword_14304BF00;					// Unknown class pointer
		ID3D11Device *m_Device;
		HWND m_Window;

		//
		// These are pools for efficient data uploads to the GPU. Each frame can use any buffer as long as there
		// is sufficient space. If there's no space left, delay execution until m_DynamicVertexBufferAvailQuery[] says a buffer
		// is no longer in use.
		//
		ID3D11Buffer		*m_DynamicVertexBuffers[3];			// DYNAMIC (VERTEX | INDEX) CPU_ACCESS_WRITE
		uint32_t			m_CurrentDynamicVertexBuffer;

		uint32_t			m_CurrentDynamicVertexBufferOffset;	// Used in relation with m_DynamicVertexBufferAvailQuery[]
		ID3D11Buffer		*m_SharedParticleIndexBuffer;		// DEFAULT INDEX CPU_ACCESS_NONE
		ID3D11Buffer		*m_SharedParticleStaticBuffer;		// DEFAULT VERTEX CPU_ACCESS_NONE
		ID3D11InputLayout	*m_ParticleShaderInputLayout;
		ID3D11InputLayout	*m_UnknownInputLayout2;
		uint32_t			m_UnknownCounter;					// 0 to 63
		uint32_t			m_UnknownCounter2;					// No limits
		void				*m_UnknownStaticBuffer[64];
		uint32_t			m_UnknownCounter3;					// 0 to 5
		bool				m_DynamicEventQueryFinished[3];
		ID3D11Query			*m_DynamicVertexBufferAvailQuery[3];// D3D11_QUERY_EVENT (Waits for a series of commands to finish execution)

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
	};

	class RendererWindow
	{
	public:
		HWND hWnd;
		int iWindowX;
		int iWindowY;
		int uiWindowWidth;
		int uiWindowHeight;
		IDXGISwapChain *pSwapChain;
		uint32_t SwapChainRenderTarget;
		char _pad0[0x2C];
	};
	static_assert(sizeof(RendererWindow) == 0x50);

	class RendererData
	{
	public:
		char _pad0[0x22];
		bool bReadOnlyDepth;// bReadOnlyStencil?
		char _pad1[0x15];
		ID3D11Device *pDevice;
		ID3D11DeviceContext2 *pContext;
		RendererWindow RenderWindowA[32];
		RenderTargetData pRenderTargets[RENDER_TARGET_COUNT];
		DepthStencilData pDepthStencils[DEPTH_STENCIL_COUNT];
		CubemapRenderTargetData pCubemapRenderTargets[RENDER_TARGET_CUBEMAP_COUNT];
		Texture3DTargetData pTexture3DRenderTargets[TEXTURE3D_COUNT];
		float ClearColor[4];
		uint8_t ClearStencil;
		CRITICAL_SECTION RendererLock;
	};
	static_assert_offset(RendererData, bReadOnlyDepth, 0x22);
	static_assert_offset(RendererData, pDevice, 0x38);
	static_assert_offset(RendererData, pContext, 0x40);
	static_assert_offset(RendererData, RenderWindowA, 0x48);
	static_assert_offset(RendererData, pRenderTargets, 0xA48);
	static_assert_offset(RendererData, pDepthStencils, 0x1FA8);
	static_assert_offset(RendererData, pCubemapRenderTargets, 0x26C8);
	static_assert_offset(RendererData, pTexture3DRenderTargets, 0x2708);
	static_assert_offset(RendererData, ClearColor, 0x2768);
	static_assert_offset(RendererData, ClearStencil, 0x2778);
	static_assert_offset(RendererData, RendererLock, 0x2780);

	class RendererShadowState
	{
	public:
		uint32_t m_StateUpdateFlags;						// Flags +0x0  0xFFFFFFFF; global state updates
		uint32_t m_PSResourceModifiedBits;					// Flags +0x4  0xFFFF
		uint32_t m_PSSamplerModifiedBits;					// Flags +0x8  0xFFFF
		uint32_t m_CSResourceModifiedBits;					// Flags +0xC  0xFFFF
		uint32_t m_CSSamplerModifiedBits;					// Flags +0x10 0xFFFF
		uint32_t m_CSUAVModifiedBits;						// Flags +0x14 0xFF

		uint32_t m_RenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
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

		uint64_t m_VertexDesc;
		VertexShader *m_CurrentVertexShader;
		PixelShader *m_CurrentPixelShader;
		D3D11_PRIMITIVE_TOPOLOGY m_Topology;

		NiPoint3 m_PosAdjust;
		NiPoint3 m_PreviousPosAdjust;
		ViewData m_CameraData;

		uint32_t m_AlphaBlendModeExtra;
		char _pad0[0xC];
	};

	class Renderer
	{
	public:
		char _pad0[0x10];
		RendererData Data;

		inline AutoPtr(HACK_Globals, Globals, 0x304BEF0);
		static Renderer *QInstance();

		void Initialize();
		void OnNewFrame();

		void BeginEvent(wchar_t *Marker) const;
		void EndEvent() const;
		void SetResourceName(ID3D11DeviceChild *Resource, const char *Format, ...);

		//
		// Resource creation/destruction
		//
		void CreateRenderTarget(uint32_t TargetIndex, const char *Name, const RenderTargetProperties *Properties);
		void CreateDepthStencil(uint32_t TargetIndex, const char *Name, const DepthStencilTargetProperties *Properties);
		void CreateCubemapRenderTarget(uint32_t TargetIndex, const char *Name, const CubeMapRenderTargetProperties *Properties);
		void DestroyRenderTarget(uint32_t TargetIndex);
		void DestroyDepthStencil(uint32_t TargetIndex);
		void DestroyCubemapRenderTarget(uint32_t TargetIndex);

		//
		// Drawing
		//
		void DrawLineShape(LineShape *GraphicsLineShape, uint32_t StartIndex, uint32_t Count);
		void DrawTriShape(TriShape *GraphicsTriShape, uint32_t StartIndex, uint32_t Count);
		void DrawDynamicTriShapeUnknown(DynamicTriShape *Shape, DynamicTriShapeDrawData *DrawData, uint32_t IndexStartOffset, uint32_t TriangleCount);
		void DrawDynamicTriShape(DynamicTriShapeData *ShapeData, DynamicTriShapeDrawData *DrawData, uint32_t IndexStartOffset, uint32_t TriangleCount, uint32_t VertexBufferOffset);
		void DrawParticleShaderTriShape(const void *DynamicData, uint32_t Count);

		DynamicTriShape *GetParticlesDynamicTriShape();


		void ClearDepthStencil(uint32_t ClearFlags)
		{

		}

		//
		// API state (RendererShadowState)
		//
		static void SetDirtyStates(bool IsComputeShader);
		static void FlushD3DResources();

		RendererShadowState *GetRendererShadowState() const;

		void SetClearColor(float R, float G, float B, float A);
		void RestorePreviousClearColor();

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
		// Shaders
		//
		ComPtr<ID3DBlob> CompileShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, const char *ProgramType);
		VertexShader *CompileVertexShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetConstant);
		PixelShader *CompilePixelShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines, std::function<const char *(int Index)> GetSampler, std::function<const char *(int Index)> GetConstant);
		HullShader *CompileHullShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines);
		DomainShader *CompileDomainShader(const wchar_t *FilePath, const std::vector<std::pair<const char *, const char *>>& Defines);

		void ReflectConstantBuffers(ComPtr<ID3D11ShaderReflection> Reflector, Buffer *ConstantGroups, uint32_t MaxGroups, std::function<const char *(int Index)> GetConstant, uint8_t *Offsets, uint32_t MaxOffsets);
		void ReflectSamplers(ComPtr<ID3D11ShaderReflection> Reflector, std::function<const char *(int Index)> GetSampler);
		void ValidateShaderReplacement(ID3D11PixelShader *Original, ID3D11PixelShader *Replacement);
		void ValidateShaderReplacement(ID3D11VertexShader *Original, ID3D11VertexShader *Replacement);
		void ValidateShaderReplacement(ID3D11ComputeShader *Original, ID3D11ComputeShader *Replacement);
		void ValidateShaderReplacement(void *Original, void *Replacement, const GUID& Guid);
		void RegisterShaderBytecode(void *Shader, const void *Bytecode, size_t BytecodeLength);
		const std::pair<std::unique_ptr<uint8_t[]>, size_t>& Renderer::GetShaderBytecode(void *Shader);

		//
		// Buffers
		//
		void *AllocateAndMapDynamicVertexBuffer(uint32_t Size, uint32_t *OutOffset);
		void *MapDynamicTriShapeDynamicData(BSDynamicTriShape *DynTriShape, DynamicTriShape *TriShape, DynamicTriShapeDrawData *DrawData, uint32_t VertexSize);
		void UnmapDynamicTriShapeDynamicData(DynamicTriShape *TriShape, DynamicTriShapeDrawData *DrawData);

		CustomConstantGroup GetShaderConstantGroup(uint32_t Size, ConstantGroupLevel Level);
		VertexCGroup GetShaderConstantGroup(VertexShader *Shader, ConstantGroupLevel Level);
		PixelCGroup GetShaderConstantGroup(PixelShader *Shader, ConstantGroupLevel Level);
		void FlushConstantGroup(CustomConstantGroup *Group);
		void FlushConstantGroupVSPS(VertexCGroup *VertexGroup, PixelCGroup *PixelGroup);
		void ApplyConstantGroupVS(const CustomConstantGroup *Group, ConstantGroupLevel Level);
		void ApplyConstantGroupPS(const CustomConstantGroup *Group, ConstantGroupLevel Level);
		void ApplyConstantGroupVSPS(const VertexCGroup *VertexGroup, const PixelCGroup *PixelGroup, ConstantGroupLevel Level);

		void IncRef(TriShape *Shape);
		void DecRef(TriShape *Shape);
		void IncRef(DynamicTriShape *Shape);
		void DecRef(DynamicTriShape *Shape);
	};
	static_assert_offset(Renderer, Data, 0x10);
}