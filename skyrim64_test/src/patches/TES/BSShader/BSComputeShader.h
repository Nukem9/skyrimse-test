#pragma once

#include "../../../common.h"
#include "BSShaderManager.h"

struct BSComputeBufferInfo
{
	ID3D11Buffer *m_Buffer;	// Always a nullptr
	void *m_Data;			// Static buffer inside the exe .data section. Can be null.
	uint32_t m_UnknownIndex;// Compute shaders do something special but I don't remember
							// off the top of my head. Optional buffer index?
};

struct BSComputeShader
{
	char _pad0[0x8];
	BSComputeBufferInfo CBuffer1;
	char _pad1[0x8];
	BSComputeBufferInfo CBuffer2;
	char _pad2[0x8];
	BSComputeBufferInfo CBuffer3;
	ID3D11ComputeShader *m_Shader;
	uint32_t m_Unknown;					// Probably technique dword
	uint32_t m_ShaderLength;			// Raw bytecode length
	uint8_t m_ConstantOffsets[32];		// Actual offset is multiplied by 4
										// Raw bytecode appended after this
};
static_assert(offsetof(BSComputeShader, CBuffer1) == 0x8, "");
static_assert(offsetof(BSComputeShader, CBuffer2) == 0x28, "");
static_assert(offsetof(BSComputeShader, CBuffer3) == 0x48, "");
static_assert(offsetof(BSComputeShader, m_Shader) == 0x60, "");
static_assert(offsetof(BSComputeShader, m_Unknown) == 0x68, "");
static_assert(offsetof(BSComputeShader, m_ShaderLength) == 0x6C, "");
static_assert(offsetof(BSComputeShader, m_ConstantOffsets) == 0x70, "");
static_assert(sizeof(BSComputeShader) == 0x90, "");