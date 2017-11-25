#pragma once

#include "../../zydis/include/Zydis/Zydis.h"
#include "../../xbyak/xbyak.h"
#include "../../common.h"
#include "codegen.h"
#include "d3d11_tls.h"

struct BSGraphicsRendererGlobals
{
	float			m_Viewport[4];						// CHECK THIS!!

	void *qword_14304BF00;								// Unknown class pointer

	ID3D11Device	*m_Device;
	HWND			m_Window;

	//
	// These are pools for efficient data uploads to the GPU. Each frame can use any buffer as long as there
	// is sufficient space. If there's no space left, delay execution until m_CommandListEndEvents[] says a buffer
	// is no longer in use.
	//
	ID3D11Buffer		*m_DynamicBuffers[3];			// DYNAMIC (VERTEX | INDEX) CPU_ACCESS_WRITE
	uint32_t			m_CurrentDynamicBufferIndex;

	uint32_t			m_FrameDataUsedSize;			// Use in relation with m_CommandListEndEvents[]
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

	float m_UnknownFloats1[3][4];						// Probably a matrix

	void *qword_14304C1B0[6][40];						// Wtf? (Probably a weird sampler state setup)
	void *qword_14304C930[2][3][12][2];					// Wtf?
	void *qword_14304CDB0[7][2][13][2];					// Wtf?
	ID3D11SamplerState *qword_14304D910[6][5];			// Samplers[Option1][Option2] (Used for PS and CS)

														//
														// Vertex/Pixel shader constant buffers. Set during load-time (CreateShaderBundle).
														//
	uint32_t		m_NextConstantBufferIndex;
	ID3D11Buffer	*m_ConstantBuffers1[4];				// Sizes: 3840 bytes
	ID3D11Buffer	*m_TempConstantBuffer1;				// 16 bytes
	void			*qword_14304DA30;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers2[19];			// Sizes: 16, 32, 48, ... 304 bytes
	void			*qword_14304DAD0;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers3[9];				// Sizes: 16, 32, 48, ... 144 bytes
	void			*qword_14304DB20;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers4[27];			// Sizes: 16, 32, 48, ... 432 bytes
	void			*qword_14304DC00;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers5[19];			// Sizes: 16, 32, 48, ... 304 bytes
	void			*qword_14304DCA0;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers6[19];			// Sizes: 16, 32, 48, ... 304 bytes
	void			*qword_14304DD40;					// CHECK THIS!! It's never assigned a value
	ID3D11Buffer	*m_ConstantBuffers7[39];			// Sizes: 16, 32, 48, ... 624 bytes
	ID3D11Buffer	*m_TempConstantBuffer2;				// 576 bytes
	ID3D11Buffer	*m_TempConstantBuffer3;				// 720 bytes
	ID3D11Buffer	*m_TempConstantBuffer4;				// 16 bytes

	IDXGIOutput *m_DXGIAdapterOutput;
	ID3D11DeviceContext *m_DeviceContext;

	void *m_FrameDurationStringHandle;					// "Frame Duration" but stored in their global string pool

	uint32_t dword_14304DEB0;							// Flags; probably global technique modifiers
	uint32_t m_PSResourceModifiedBits;					// Flags
	uint32_t m_PSSamplerModifiedBits;					// Flags
	uint32_t m_CSResourceModifiedBits;					// Flags
	uint32_t m_CSSamplerModifiedBits;					// Flags
	uint32_t m_CSUAVModifiedBits;						// Flags

	uint32_t dword_14304DEC8[8];

	uint32_t rshadowState_iDepthStencil;				// Index
	uint32_t rshadowState_iDepthStencilSlice;			// Index
	uint32_t iRenderTargetIndexes[5][2];				// Index[0] = Base target, Index[1] = Slice target

	char __zz0[0x50];
	float float_14304DF68;								// Possibly something to do with diffuse

	uint32_t m_PSSamplerSetting1[16];
	uint32_t m_PSSamplerSetting2[16];
	ID3D11ShaderResourceView *m_PSResources[16];

	uint32_t m_CSSamplerSetting1[16];
	uint32_t m_CSSamplerSetting2[16];

	ID3D11ShaderResourceView *m_CSResources[16];
	char __zz1[0x40];
	ID3D11UnorderedAccessView *m_CSUAVResources[8];
	char __zz2[0x2A0];
};

#define CHECK_OFFSET(member, actualAddr) static_assert(offsetof(BSGraphicsRendererGlobals, member) == (actualAddr - 0x14304BEF0), "")
static_assert(sizeof(BSGraphicsRendererGlobals) == 0x25A0, "");
CHECK_OFFSET(m_Viewport, 0x14304BEF0);
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
CHECK_OFFSET(m_UnknownFloats1, 0x14304C180);
CHECK_OFFSET(qword_14304C1B0, 0x14304C1B0);
CHECK_OFFSET(qword_14304C930, 0x14304C930);
CHECK_OFFSET(qword_14304CDB0, 0x14304CDB0);
CHECK_OFFSET(qword_14304D910, 0x14304D910);
CHECK_OFFSET(m_NextConstantBufferIndex, 0x14304DA00);
CHECK_OFFSET(m_ConstantBuffers1, 0x14304DA08);
CHECK_OFFSET(m_TempConstantBuffer1, 0x14304DA28);
CHECK_OFFSET(qword_14304DA30, 0x14304DA30);
CHECK_OFFSET(m_ConstantBuffers2, 0x14304DA38);
CHECK_OFFSET(qword_14304DAD0, 0x14304DAD0);
CHECK_OFFSET(m_ConstantBuffers3, 0x14304DAD8);
CHECK_OFFSET(qword_14304DB20, 0x14304DB20);
CHECK_OFFSET(m_ConstantBuffers4, 0x14304DB28);
CHECK_OFFSET(qword_14304DC00, 0x14304DC00);
CHECK_OFFSET(m_ConstantBuffers5, 0x14304DC08);
CHECK_OFFSET(qword_14304DCA0, 0x14304DCA0);
CHECK_OFFSET(m_ConstantBuffers6, 0x14304DCA8);
CHECK_OFFSET(qword_14304DD40, 0x14304DD40);
CHECK_OFFSET(m_ConstantBuffers7, 0x14304DD48);
CHECK_OFFSET(m_TempConstantBuffer2, 0x14304DE80);
CHECK_OFFSET(m_TempConstantBuffer3, 0x14304DE88);
CHECK_OFFSET(m_TempConstantBuffer4, 0x14304DE90);
CHECK_OFFSET(m_DXGIAdapterOutput, 0x14304DE98);
CHECK_OFFSET(m_DeviceContext, 0x14304DEA0);
CHECK_OFFSET(m_FrameDurationStringHandle, 0x14304DEA8);
CHECK_OFFSET(dword_14304DEB0, 0x14304DEB0);
CHECK_OFFSET(m_PSResourceModifiedBits, 0x14304DEB4);
CHECK_OFFSET(m_PSSamplerModifiedBits, 0x14304DEB8);
CHECK_OFFSET(m_CSResourceModifiedBits, 0x14304DEBC);
CHECK_OFFSET(m_CSSamplerModifiedBits, 0x14304DEC0);
CHECK_OFFSET(m_CSUAVModifiedBits, 0x14304DEC4);
CHECK_OFFSET(dword_14304DEC8, 0x14304DEC8);
CHECK_OFFSET(rshadowState_iDepthStencil, 0x14304DEE8);
CHECK_OFFSET(rshadowState_iDepthStencilSlice, 0x14304DEEC);
CHECK_OFFSET(iRenderTargetIndexes, 0x14304DEF0);
CHECK_OFFSET(__zz0, 0x14304DF18);
CHECK_OFFSET(float_14304DF68, 0x14304DF68);
CHECK_OFFSET(m_PSSamplerSetting1, 0x14304DF6C);
CHECK_OFFSET(m_PSSamplerSetting2, 0x14304DFAC);
CHECK_OFFSET(m_PSResources, 0x14304DFF0);
CHECK_OFFSET(m_CSSamplerSetting1, 0x14304E070);
CHECK_OFFSET(m_CSSamplerSetting2, 0x14304E0B0);
CHECK_OFFSET(m_CSResources, 0x14304E0F0);
CHECK_OFFSET(m_CSUAVResources, 0x14304E1B0);