#pragma once

#include <xbyak/xbyak.h>
#include <zydis/include/Zydis/Zydis.h>
#include "common.h"

enum class PatchType
{
	MOV_REG_MEM,
	MOV_MEM_REG,
	MOV_MEM_IMM,

	ADD_REG_MEM,
	ADD_MEM_REG,
	ADD_MEM_IMM,

	AND_REG_MEM,
	AND_MEM_REG,
	AND_MEM_IMM,

	CMP_REG_MEM,
	CMP_MEM_REG,
	CMP_MEM_IMM,

	MOVSXD_REG_MEM,
	MOVZX_REG_MEM,
	LEA_REG_MEM,
	OR_MEM_REG,
	OR_MEM_IMM,
	MOVSS_REG_MEM,
	MOVSS_MEM_REG,
	INC_MEM,
	DEC_MEM,
	SUBSS_REG_MEM,
	ADDSS_REG_MEM,
	MOVUPS_REG_MEM,
	MOVUPS_MEM_REG,
	MOVAPS_REG_MEM,
	MOVAPS_MEM_REG,
	SHUFPS_REG_MEM,
	MOVSD_REG_MEM,
	MOVSD_MEM_REG,
	XADD_MEM_REG,
};

struct PatchEntry
{
	PatchType Type;
	uintptr_t ExeOffset;
	union
	{
		uint64_t Immediate;
		ZydisRegister Register;
	};
	uintptr_t Offset;
	int MemSize;

	ZydisRegister Base;
	ZydisRegister Index;
	uint32_t Scale;
};

struct OpTableEntry
{
	ZydisMnemonic Mnemonic;
	ZydisOperandType Operand1;
	ZydisOperandType Operand2;
	const char *OutputType;
};

struct PatchCodeGen : public Xbyak::CodeGenerator
{
public:
	PatchCodeGen(const PatchEntry *Patch, uintptr_t Memory, size_t MemorySize);

private:
	void SetTlsBase(const Xbyak::Reg64& Register);

	const Xbyak::Reg64& GetFreeScratch(ZydisRegister Operand, ZydisRegister Base, ZydisRegister Index);
	const Xbyak::AddressFrame& MemOpSize(int BitSize);
	const Xbyak::Reg& ZydisToXbyak(ZydisRegister Register);
	const Xbyak::Reg64& ZydisToXbyak64(ZydisRegister Register);
	const Xbyak::Xmm& ZydisToXbyakXmm(ZydisRegister Register);
};

void CreateXbyakPatches();
void CreateXbyakCodeBlock();
void GenerateInstruction(uintptr_t Address);
void GenerateCommonInstruction(ZydisDecodedInstruction *Instruction, ZydisDecodedOperand *Operands, const char *Type);
void WriteCodeHook(uintptr_t TargetAddress, void *Code);
uint32_t crc32c(unsigned char *Data, size_t Len);