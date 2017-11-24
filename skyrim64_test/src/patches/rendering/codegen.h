#pragma once

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

const uintptr_t XrefList[] =
{
#include "d3d11_tls_xreflist.inl"
};

const PatchEntry XrefGeneratedPatches[] =
{
#define DO_THREADING_PATCH_REG(TYPE, EXE_OFFSET, REG, OFFSET, MEM_SIZE, B, I, S) { PatchType::##TYPE, EXE_OFFSET, ZYDIS_REGISTER_##REG, OFFSET, MEM_SIZE, ZYDIS_REGISTER_##B, ZYDIS_REGISTER_##I, S },
#define DO_THREADING_PATCH_IMM(TYPE, EXE_OFFSET, IMM, OFFSET, MEM_SIZE, B, I, S) { PatchType::##TYPE, EXE_OFFSET, IMM, OFFSET, MEM_SIZE, ZYDIS_REGISTER_##B, ZYDIS_REGISTER_##I, S },
#define DO_SHUFPS_FIXUP(EXE_OFFSET, IMM)

#include "d3d11_tls_patchlist.inl"

#undef DO_SHUFPS_FIXUP
#undef DO_THREADING_PATCH_REG
#undef DO_THREADING_PATCH_IMM
};

static_assert(std::extent<decltype(XrefGeneratedPatches)>::value == std::extent<decltype(XrefList)>::value, "WARNING: Array sizes differ");

const std::unordered_map<uintptr_t, int> XrefGeneratedShufps
({
#define DO_THREADING_PATCH_REG(TYPE, EXE_OFFSET, REG, OFFSET, MEM_SIZE, B, I, S)
#define DO_THREADING_PATCH_IMM(TYPE, EXE_OFFSET, IMM, OFFSET, MEM_SIZE, B, I, S)
#define DO_SHUFPS_FIXUP(EXE_OFFSET, IMM) { EXE_OFFSET, IMM },

#include "d3d11_tls_patchlist.inl"

#undef DO_SHUFPS_FIXUP
#undef DO_THREADING_PATCH_REG
#undef DO_THREADING_PATCH_IMM
});

const std::vector<OpTableEntry> OpTable =
{
	{ ZYDIS_MNEMONIC_MOV, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOV_REG_MEM" },		// mov reg, [memory]
	{ ZYDIS_MNEMONIC_MOV, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOV_MEM_REG" },		// mov [memory], reg
	{ ZYDIS_MNEMONIC_MOV, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "MOV_MEM_IMM" },		// mov [memory], imm

	{ ZYDIS_MNEMONIC_ADD, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "ADD_REG_MEM" },		// add reg, [memory]
	{ ZYDIS_MNEMONIC_ADD, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "ADD_MEM_REG" },		// add [memory], reg
	{ ZYDIS_MNEMONIC_ADD, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "ADD_MEM_IMM" },		// add [memory], imm

	{ ZYDIS_MNEMONIC_AND, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "AND_REG_MEM" },		// and reg, [memory]
	{ ZYDIS_MNEMONIC_AND, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "AND_MEM_REG" },		// and [memory], reg
	{ ZYDIS_MNEMONIC_AND, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "AND_MEM_IMM" },		// and [memory], imm

	{ ZYDIS_MNEMONIC_CMP, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "CMP_REG_MEM" },		// cmp reg, [memory]
	{ ZYDIS_MNEMONIC_CMP, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "CMP_MEM_REG" },		// cmp [memory], reg
	{ ZYDIS_MNEMONIC_CMP, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "CMP_MEM_IMM" },		// cmp [memory], imm

	{ ZYDIS_MNEMONIC_MOVSXD, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVSXD_REG_MEM" },// movsxd reg, [memory]

	{ ZYDIS_MNEMONIC_MOVZX, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVZX_REG_MEM" },	// movzx reg, [memory]

	{ ZYDIS_MNEMONIC_LEA, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "LEA_REG_MEM" },		// lea reg, [memory]

	{ ZYDIS_MNEMONIC_OR, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "OR_MEM_REG" },		// or [memory], reg
	{ ZYDIS_MNEMONIC_OR, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_IMMEDIATE, "OR_MEM_IMM" },		// or [memory], imm

	{ ZYDIS_MNEMONIC_MOVSS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVSS_REG_MEM" },	// movss reg, [memory]
	{ ZYDIS_MNEMONIC_MOVSS, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOVSS_MEM_REG" },	// movss [memory], reg

	{ ZYDIS_MNEMONIC_INC, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_UNUSED, "INC_MEM" },			// inc [memory]

	{ ZYDIS_MNEMONIC_DEC, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_UNUSED, "DEC_MEM" },			// dec [memory]

	{ ZYDIS_MNEMONIC_SUBSS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "SUBSS_REG_MEM" },	// subss reg, [memory]

	{ ZYDIS_MNEMONIC_ADDSS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "ADDSS_REG_MEM" },	// addss reg, [memory]

	{ ZYDIS_MNEMONIC_MOVUPS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVUPS_REG_MEM" },// movups reg, [memory]
	{ ZYDIS_MNEMONIC_MOVUPS, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOVUPS_MEM_REG" },// movups [memory], reg

	{ ZYDIS_MNEMONIC_MOVAPS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVAPS_REG_MEM" },// movaps reg, [memory]
	{ ZYDIS_MNEMONIC_MOVAPS, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOVAPS_MEM_REG" },// movaps [memory], reg

	{ ZYDIS_MNEMONIC_SHUFPS, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "SHUFPS_REG_MEM" },// shufps reg, [memory], imm8

	{ ZYDIS_MNEMONIC_MOVSD, ZYDIS_OPERAND_TYPE_REGISTER, ZYDIS_OPERAND_TYPE_MEMORY, "MOVSD_REG_MEM" },  // movsd xmm, [memory]
	{ ZYDIS_MNEMONIC_MOVSD, ZYDIS_OPERAND_TYPE_MEMORY, ZYDIS_OPERAND_TYPE_REGISTER, "MOVSD_MEM_REG" },  // movsd [memory], xmm
};