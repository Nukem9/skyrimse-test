#include "../rendering/common.h"
#include "../../common.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "MemoryContextTracker.h"
#include "BSSpinLock.h"
#include "BSBatchRenderer.h"

AutoPtr<BYTE, 0x31F54CD> byte_1431F54CD;
AutoPtr<DWORD, 0x1E32FDC> dword_141E32FDC;

unsigned __int64 *sub_141320370(__int64 a1, unsigned __int64 *a2);
signed __int64 *sub_1413203D0(__int64 a1, signed __int64 *a2);

struct RTTIBaseClassArray
{
	DWORD arrayOfBaseClassDescriptors[]; // RTTIBaseClassDescriptor *
};

struct RTTICompleteObjectLocator
{
	DWORD signature;		// 32-bit zero, 64-bit one, until loaded
	DWORD offset;			// Offset of this vtable in the complete class
	DWORD cdOffset;			// Constructor displacement offset
	DWORD typeDescriptor;	// TypeDescriptor of the complete class
	DWORD classDescriptor;	// Describes inheritance hierarchy
};

struct RTTIClassHierarchyDescriptor
{
	DWORD signature;		// Always zero or one
	DWORD attributes;		// Flags
	DWORD numBaseClasses;	// Number of classes in baseClassArray
	DWORD baseClassArray;	// RTTIBaseClassArray
};

struct PMD
{
	int mdisp;	// Member displacement (vftable offset in the class itself)
	int pdisp;	// Vbtable displacement (vbtable offset, -1: vftable is at displacement PMD.mdisp inside the class)
	int vdisp;	// Displacement inside vbtable
};

struct RTTIBaseClassDescriptor
{
	DWORD typeDescriptor;		// Type descriptor of the class
	DWORD numContainedBases;	// Number of nested classes following in the Base Class Array
	PMD disp;					// Pointer-to-member displacement info
	DWORD attributes;			// Flags
};

#define RTTI_BSShaderProperty 0x19A0F78
#define RTTI_BSBatchRenderer 0x19AC298
#define RTTI_BSShaderAccumulator 0x19A9FF0
#define RTTI_BSShader 0x19AD110
#define RTTI_BSShaderMaterial 0x19AD098

void AssertIsRTTIType(uintptr_t Object, uintptr_t RTTIInfo)
{
	__try
	{
		RTTIInfo += g_ModuleBase;
		RTTICompleteObjectLocator *rttiCOL = *(RTTICompleteObjectLocator **)(*(uintptr_t *)Object - sizeof(void *));
		RTTICompleteObjectLocator *otherrtti = (RTTICompleteObjectLocator *)RTTIInfo;

		if (rttiCOL->typeDescriptor == otherrtti->typeDescriptor)
			return;

		// Loop through the base classes and check for a match
		auto object = rttiCOL;
		auto hierarchy = (RTTIClassHierarchyDescriptor *)(g_ModuleBase + object->classDescriptor);
		auto baseClassArray = (RTTIBaseClassArray *)(g_ModuleBase + hierarchy->baseClassArray);

		for (DWORD i = 0; i < hierarchy->numBaseClasses; i++)
		{
			auto baseClassInfo = (RTTIBaseClassDescriptor *)(g_ModuleBase + baseClassArray->arrayOfBaseClassDescriptors[i]);

			if (baseClassInfo->typeDescriptor == otherrtti->typeDescriptor)
				return;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

	__debugbreak();
}

void operator_delete(__int64 a1, __int64 a2)
{
	((void(__fastcall *)(void *))(g_ModuleBase + 0x1026F0))((void *)a1);
}

void sub_1401C49C0(__int64 a1, __int64(__fastcall *a2)(uint64_t, __int64), __int64 a3)
{
	__int64 v3; // r9
	__int64(__fastcall *v4)(uint64_t, __int64); // r8
	__int64 v5; // rbx
	__int64 v6; // rax
	__int64 v7; // rsi
	__int64 v8; // rdi
	__int64(__fastcall *v9)(uint64_t, __int64); // [rsp+58h] [rbp+10h]
	__int64 v10; // [rsp+60h] [rbp+18h]

	v10 = a3;
	v9 = a2;
	v3 = a3;
	v4 = a2;
	v5 = a1;
	if (*(uint64_t *)(a1 + 8))
	{
		while (1)
		{
			v6 = *(uint64_t *)(v5 + 8);
			v7 = *(uint64_t *)(v6 + 8);
			*(uint64_t *)(v6 + 8) = 0i64;
			if (v4)
				break;
			v8 = *(uint64_t *)(v5 + 8);
			if (v8)
			{
				sub_1401C49C0(*(uint64_t *)(v5 + 8), 0i64, 0i64);
				operator_delete(v8, 16i64);
				goto LABEL_6;
			}
		LABEL_7:
			*(uint64_t *)(v5 + 8) = v7;
			if (!v7)
			{
				*(DWORD *)v5 = 0;
				return;
			}
		}
		v4(*(uint64_t *)(v5 + 8), v3);
	LABEL_6:
		v3 = v10;
		v4 = v9;
		goto LABEL_7;
	}
	*(DWORD *)a1 = 0;
}

void sub_14131F910(__int64 a1, __int64 a2)
{
	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	if (a2)
	{
		BSSpinLock& lock = *(BSSpinLock *)(a2 + 8);

		lock.Acquire();
		*(uint64_t *)(a1 + 8) = *(uint64_t *)a2;
		*(uint64_t *)a2 = a1;
		lock.Release();
	}
	else if (a1)
	{
		sub_1401C49C0(a1, 0i64, 0i64);
		operator_delete(a1, (unsigned int)(a2 + 16));
	}
}

void sub_14131ED70(BSRenderPass *a1, unsigned int a2, unsigned __int8 a3, unsigned int a4)
{
	__int64 v6; // r14
	__int64 result; // rax
	__int64 v11; // rdi

	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	auto sub_14131EFF0 = (__int64(__fastcall *)(unsigned int a1, __int64 a2))(g_ModuleBase + 0x131F350);
	auto sub_14131F450 = (__int64(__fastcall *)(BSRenderPass *a1, __int64 a2, unsigned int a3))(g_ModuleBase + 0x131F7B0);
	auto sub_14131F1F0 = (__int64(__fastcall *)(BSRenderPass *a1, char a2, unsigned int a3))(g_ModuleBase + 0x131F550);
	auto sub_14131F2A0 = (__int64(__fastcall *)(BSRenderPass *a1, unsigned __int8 a2, unsigned int a3))(g_ModuleBase + 0x131F600);

	v6 = (uint64_t)a1->m_Shader;

	if (dword_1432A8214 == a2 && a2 != 0x5C006076 && v6 == qword_1432A8218
		|| (dword_141E32FDC = a2, result = sub_14131EFF0(a2, v6), (BYTE)result))
	{
		BSShaderProperty *property = a1->m_Property;

		if (property)
			v11 = *(uint64_t *)((uint64_t)property + 120);// v11 = BSShaderProperty ---- (v11 + 120) = BSShaderMaterial *
		else
			v11 = 0;

		AssertIsRTTIType((uint64_t)a1->m_Property, RTTI_BSShaderProperty);
		AssertIsRTTIType(v11, RTTI_BSShaderMaterial);
		AssertIsRTTIType(v6, RTTI_BSShader);

		if (v11 != qword_1434B5220)
		{
			if (v11)
				a1->m_Shader->SetupMaterial((BSShaderMaterial *)v11);

			qword_1434B5220 = v11;
		}

		*(BYTE *)(a1->m_Geometry + 264) = a1->Byte1E;

		if (*(uint64_t *)(a1->m_Geometry + 304))// v8 + 304 == BSGeometry::QSkinPartitions(). NiSkinPartition?
			sub_14131F2A0(a1, a3, a4);// Something to do with skinning, "Render Error : Skin instance is nullptr"
		else if (*(BYTE *)(a1->m_Geometry + 265) & 8)// Possibly BSGeometry::NeedsCustomRender
			sub_14131F450(a1, a3, a4);
		else
			sub_14131F1F0(a1, a3, a4); // v9 = Render pass, *(v9 + 16) = Render pass geometry, *(v9) = BSShader
	}
}

void sub_14131F9F0(__int64 *a1, unsigned int a2)
{
	unsigned int v4; // ebp
	__int64 *v5; // rdi
	__int64 v7; // rbx
	unsigned int v8; // edx
	__int64 v9; // rcx
	bool v10; // r8

	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	v4 = a2;
	v5 = a1;
	if (*a1)
	{
		sub_14131F090();

		v7 = *v5;
		if (*v5)
		{
			while (1)
			{
				v8 = *(DWORD *)(v7 + 24);
				if (*(uint64_t *)(v7 + 16))
					break;
			LABEL_18:
				v7 = *(uint64_t *)(v7 + 48);
				if (!v7)
					goto LABEL_19;
			}
			if ((v4 & 0x108) == 0)
			{
				if (*(uint64_t *)(*(uint64_t *)(v7 + 8) + 56i64) & 0x1000000000i64)// BSShaderProperty::VertexDesc?
					BSGraphics__Renderer__RasterStateSetCullMode(0);
				else
					BSGraphics__Renderer__RasterStateSetCullMode(1);
			}

			v9 = *(uint64_t *)(*(uint64_t *)(v7 + 16) + 288i64);// BSGeometry::GetModelBound?
			v10 = v9 && (*(WORD *)(v9 + 48) >> 9) & 1;
			sub_14131ED70((BSRenderPass *)v7, v8, v10, v4);
			goto LABEL_18;
		}
	LABEL_19:
		if (!(v4 & 0x108))
			BSGraphics__Renderer__RasterStateSetCullMode(1);

		*v5 = 0i64;
		v5[1] = 0i64;

		sub_14131F090();
	}
}

__int64 sub_141320350(__int64 a1, __int64 a2)
{
	__int64 v2; // rbx

	v2 = a2;
	sub_141320370(a1 + 8, (unsigned __int64 *)a2);
	return v2;
}

unsigned __int64 *sub_141320370(__int64 a1, unsigned __int64 *a2)
{
	__int64 v2; // r9
	unsigned __int64 v3; // rax
	unsigned __int64 v4; // r8
	unsigned __int64 *result; // rax

	v2 = *(uint64_t *)(a1 + 32);
	v3 = 0i64;
	v4 = 0i64;
	if (v2)
	{
		v3 = *(uint64_t *)(a1 + 32);
		v4 = v2 + 16i64 * *(unsigned int *)(a1 + 4);
		while (v3 < v4 && !*(uint64_t *)(v3 + 8))
			v3 += 16i64;
	}
	*a2 = v3;
	result = a2;
	a2[1] = v4;
	return result;
}

__int64 sub_1413203B0(__int64 a1, __int64 a2)
{
	__int64 v2; // rbx

	v2 = a2;
	sub_1413203D0(a1 + 8, (signed __int64 *)a2);
	return v2;
}

signed __int64 *sub_1413203D0(__int64 a1, signed __int64 *a2)
{
	__int64 v2; // r8
	signed __int64 v3; // rax

	v2 = *(uint64_t *)(a1 + 32);
	if (v2)
	{
		v3 = v2 + 16i64 * *(unsigned int *)(a1 + 4);
		*a2 = v3;
		a2[1] = v3;
	}
	else
	{
		*a2 = 0i64;
		a2[1] = 0i64;
	}
	return a2;
}

void BSBatchRenderer::PassInfo::Render(unsigned int a2)
{
	//auto Render = (void(__fastcall *)(__int64 a1, unsigned int a2))(g_ModuleBase + 0x131CB70);

	__int64 a1 = (__int64)this;

	__int64 v4; // r14
	__int64 v6; // rbx
	__int64 v7; // rax
	__int64 v8; // rsi
	__int64 v9; // rdi

	v4 = a1;
	if (this->UnkByte1 & 1)
	{
		sub_14131F9F0((__int64 *)(a1 + 8), a2);
	}
	else
	{
		if (!this->m_BatchRenderer)
			goto LABEL_14;

		uint64_t v5 = (uint64_t)this->m_BatchRenderer;

		(*(void(__fastcall **)(__int64, signed __int64, signed __int64, uint64_t, signed __int64))(*(uint64_t *)v5 + 24i64))(
			v5,
			1i64,
			BSSM_BLOOD_SPLATTER,
			a2,
			-2i64);
	}

	v6 = *(uint64_t *)v4;

	if (*(uint64_t *)v4)
	{
		if (!*(BYTE *)(v6 + 108))
			return;
		if (*(uint64_t *)(v6 + 96))
		{
			do
			{
				v7 = *(uint64_t *)(v6 + 96);
				v8 = *(uint64_t *)(v7 + 8);
				*(uint64_t *)(v7 + 8) = 0i64;
				if (true /*sub_14131F910*/)
				{
					sub_14131F910(*(uint64_t *)(v6 + 96), g_ModuleBase + 0x34B5230);
				}
				else
				{
					v9 = *(uint64_t *)(v6 + 96);
					if (v9)
					{
						sub_1401C49C0(*(uint64_t *)(v6 + 96), 0i64, 0i64);
						operator_delete(v9, 16i64);
					}
				}
				*(uint64_t *)(v6 + 96) = v8;
			} while (v8);
		}
		*(DWORD *)(v6 + 88) = 0;
	}

LABEL_14:
	this->UnkWord1 = 0;
}

void BSBatchRenderer::PassInfo::Unregister()
{
	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	this->UnkPtr2 = 0;
	this->UnkPtr3 = 0;

	if (this->m_BatchRenderer)
		sub_14131D6E0((__int64)this->m_BatchRenderer);

	this->UnkWord1 = 0;
}

bool BSBatchRenderer::HasTechniquePasses(uint32_t StartTech, uint32_t EndTech)
{
	auto sub_14131F100 = (char(__fastcall *)(__int64 a1, unsigned int a2, unsigned int a3))(g_ModuleBase + 0x131F460);
	return sub_14131F100((__int64)this, StartTech, EndTech);
}

bool BSBatchRenderer::sub_14131E8F0(unsigned int a2, signed int *a3)
{
	__int64 a1 = (__int64)this;

	AssertIsRTTIType(a1, RTTI_BSBatchRenderer);

	signed int v4; // eax
	bool v5; // zf
	signed __int64 v6; // r8

	if ((unsigned int)*a3 > 4)
		*a3 = 0;
	v4 = *a3;
	v5 = *a3 == 5;
	if (*a3 < 5)
	{
		v6 = *a3;
		do
		{
			// BSTArray
			if (this->m_RenderGroups[a2].m_Pass[v6])
			{
				*a3 = v4;
				v4 = 5;
				v6 = 5i64;
			}
			++v4;
			++v6;
			v5 = v4 == 5;
		} while (v4 < 5);
	}
	if (v5)
		*a3 = 0;
	return v4 == 6;
}

char BSBatchRenderer::sub_14131E700(uint32_t *a2, __int64 a3, __int64 a4)
{
	__int64 a1 = (__int64)this;

	int v4; // er10
	unsigned int v5; // er11
	__int64 v6; // rbp
	__int64 v7; // rdi
	uint32_t *v8; // rsi
	__int64 v9; // rbx
	__int64 v10; // rdx
	signed __int64 v11; // rax

	v4 = *a2;
	v5 = 0;
	v6 = a4;
	v7 = a3;
	v8 = a2;
	v9 = a1;
	if (!*a2)
		return sub_14131E7B0(a2, (signed int *)a3, (__int64 *)a4);

	// Inlined function here
	v10 = *(uint64_t *)(a1 + 72);
	if (v10)
	{
		v11 = v10 + 16i64 * (v4 & (unsigned int)(*(DWORD *)(a1 + 44) - 1));
		if (*(uint64_t *)(v11 + 8))
		{
			while (*(DWORD *)v11 != v4)
			{
				v11 = *(uint64_t *)(v11 + 8);
				if (v11 == *(uint64_t *)(a1 + 56))
					goto LABEL_8;
			}
			v5 = *(DWORD *)(v11 + 4);
		}
	}

LABEL_8:
	if (!sub_14131E8F0(v5, (signed int *)a3))
	{
		a4 = v6;
		a3 = v7;
		a2 = v8;
		a1 = v9;
		return sub_14131E7B0(a2, (signed int *)a3, (__int64 *)a4);
	}
	return 1;
}

char BSBatchRenderer::sub_14131ECE0(uint32_t *a2, __int64 a3, __int64 a4)
{
	__int64 a1 = (__int64)this;

	__int64 v4; // r11
	__int64 v5; // r10
	unsigned int v6; // ebx
	signed __int64 v7; // rax

	v4 = *(uint64_t *)(a1 + 72);
	v5 = a1;
	v6 = 0;
	if (v4)
	{
		// BSTScatterTable
		v7 = v4 + 16i64 * (*a2 & (unsigned int)(*(DWORD *)(a1 + 44) - 1));
		if (*(uint64_t *)(v7 + 8))
		{
			while (*(DWORD *)v7 != *a2)
			{
				v7 = *(uint64_t *)(v7 + 8);
				if (v7 == *(uint64_t *)(a1 + 56))
					goto LABEL_7;
			}
			v6 = *(DWORD *)(v7 + 4);
		}
	}
LABEL_7:
	if (*(BYTE *)(a1 + 108))
		this->m_RenderGroups[v6].Clear(true);

	return sub_14131E700(a2, a3, a4);
}

bool BSBatchRenderer::sub_14131E7B0(uint32_t *a2, signed int *a3, __int64 *a4)
{
	signed int *v4; // r14
	uint32_t *v5; // rsi
	__int64 v6; // rbx
	__int64 v7; // rax
	unsigned int v8; // ecx
	unsigned int v9; // ebp
	__int64 v10; // rdi
	__int64 v11; // r8
	signed __int64 v12; // rcx
	bool i; // zf

	__int64 a1 = (__int64)this;

	v4 = a3;
	v5 = a2;
	v6 = a1;
	v7 = *a4;
	if (!*a4 || !*(uint64_t *)(v7 + 8) && !*(DWORD *)v7)
		return 0;
	v8 = *(DWORD *)v7;
	for (*a2 = *(DWORD *)v7; v8 < *(DWORD *)(v6 + 80); *a2 = *(DWORD *)v7)
	{
		v7 = *(uint64_t *)(v7 + 8);
		if (!v7)
			return 0;
		v8 = *(DWORD *)v7;
	}
	if (v8 > *(DWORD *)(v6 + 84))
		return 0;
	v9 = 0;
	// "RenderPasses in active lists are out of order, passes will probably be leaked"
	if (*(BYTE *)(v6 + 108))
	{
		v10 = *(uint64_t *)(v6 + 96);
		if (v10)
		{
			*(uint64_t *)(v6 + 96) = *(uint64_t *)(v10 + 8);
			*(DWORD *)(v6 + 88) = *(DWORD *)v10;
			*(uint64_t *)(v10 + 8) = 0i64;
			if (true /*sub_14131F910*/)
			{
				sub_14131F910(v10, g_ModuleBase + 0x34B5230);
			}
			else
			{
				sub_1401C49C0(v10, 0i64, 0i64);
				operator_delete(v10, 16i64);
			}
		}
		else
		{
			*(DWORD *)(v6 + 88) = 0;
		}
	}
	else
	{
		*a4 = *(uint64_t *)(*a4 + 8);
	}
	v11 = *(uint64_t *)(v6 + 72);
	if (v11)
	{
		// BSTScatterTable
		v12 = v11 + 16i64 * (*v5 & (unsigned int)(*(DWORD *)(v6 + 44) - 1));
		for (i = *(uint64_t *)(v12 + 8) == 0i64; !i; i = v12 == *(uint64_t *)(v6 + 56))
		{
			if (*(DWORD *)v12 == *v5)
			{
				v9 = *(DWORD *)(v12 + 4);
				return sub_14131E8F0(v9, v4);
			}
			v12 = *(uint64_t *)(v12 + 8);
		}
	}
	return sub_14131E8F0(v9, v4);
}

char BSBatchRenderer::sub_14131E960(unsigned int *a2, unsigned int *a3, __int64 a4, unsigned int a5)
{
	auto& GraphicsGlobals = *(BSGraphicsRendererGlobals *)GetThreadedGlobals();

	__int64 a1 = (__int64)this;

	__int64 v5; // r11
	unsigned int v6; // ebx
	signed __int64 v10; // r9
	unsigned __int8 v11; // di
	bool v13; // r10

	v5 = *(uint64_t *)(a1 + 72);
	v6 = 0;

	AssertIsRTTIType(a1, RTTI_BSBatchRenderer);

	// BSTScatterTree
	if (v5)
	{
		v10 = v5 + 16i64 * (*(DWORD *)a2 & (unsigned int)(*(DWORD *)(a1 + 44) - 1));
		if (*(uint64_t *)(v10 + 8))
		{
			while (*(DWORD *)v10 != *(DWORD *)a2)
			{
				v10 = *(uint64_t *)(v10 + 8);
				if (v10 == *(uint64_t *)(a1 + 56))
					goto LABEL_7;
			}
			v6 = *(DWORD *)(v10 + 4);
		}
	}
LABEL_7:
	v11 = 0;
	v13 = (a5 & 0x108) != 0;
	if (*a3 == 0)
	{
		if (!v13)
			BSGraphics__Renderer__RasterStateSetCullMode(1);

		BSGraphics__Renderer__SetUseScrapConstantValue(false);
		BSGraphics__Renderer__AlphaBlendStateSetUnknown1(0);
	}
	else if (*a3 == 2)
	{
		if (!v13)
			BSGraphics__Renderer__RasterStateSetCullMode(0);

		BSGraphics__Renderer__SetUseScrapConstantValue(false);
		BSGraphics__Renderer__AlphaBlendStateSetUnknown1(0);
	}
	else if (*a3 == 3)
	{
		if (!v13)
			BSGraphics__Renderer__RasterStateSetCullMode(0);

		BSGraphics__Renderer__SetUseScrapConstantValue(true);

		v11 = 1;

		if (byte_1431F54CD)
			BSGraphics__Renderer__AlphaBlendStateSetUnknown1(1);
	}
	else if (*a3 == 1)
	{
		if (!v13)
			BSGraphics__Renderer__RasterStateSetCullMode(1);

		BSGraphics__Renderer__SetUseScrapConstantValue(true);

		v11 = 1;

		if (byte_1431F54CD)
			BSGraphics__Renderer__AlphaBlendStateSetUnknown1(1);
	}
	else if (*a3 == 4)
	{
		if (!v13)
			BSGraphics__Renderer__RasterStateSetCullMode(1);

		BSGraphics__Renderer__SetUseScrapConstantValue(true);
		v11 = 1;
		BSGraphics__Renderer__AlphaBlendStateSetUnknown1(0);
	}
	else
	{
		__debugbreak();
	}

	RenderPassGroup& group = this->m_RenderGroups[v6];
	BSRenderPass *pass = group.m_Pass[*a3];

	if (pass)
	{
		while (pass)
		{
			sub_14131ED70(pass, *a2, v11, a5);
			pass = (BSRenderPass *)((uint64_t *)pass)[6];
		}
	}

	if (*(BYTE *)(a1 + 108))
	{
		__int64 v20 = *a3;

		if (v20 < 0 || v20 >= ARRAYSIZE(group.m_Pass))
			__debugbreak();

		group.UnkDword1 &= ~(1 << v20);
		group.m_Pass[v20] = nullptr;
	}

	sub_14131F090();

	BSGraphics__Renderer__AlphaBlendStateSetUnknown1(0);
	++*a3;
	return sub_14131E700((uint32_t *)a2, (__int64)a3, a4);
}

void BSBatchRenderer::sub_14131D6E0(__int64 a1)
{
	__int64 v1; // rbp
	__int64 v10; // rax
	__int64 v11; // rsi
	__int64 v12; // rdi

	AssertIsRTTIType(a1, RTTI_BSBatchRenderer);

	auto *batcher = (BSBatchRenderer *)a1;

	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	for (auto itr = batcher->m_TechToGroup.begin(); itr != batcher->m_TechToGroup.end(); itr++)
	{
		if (itr)
			batcher->m_RenderGroups[*itr].Clear(true);
	}

	// This entire block is some inlined function
	v1 = a1;

	if (*(uint64_t *)(v1 + 96))
	{
		do
		{
			v10 = *(uint64_t *)(v1 + 96);
			v11 = *(uint64_t *)(v10 + 8);
			*(uint64_t *)(v10 + 8) = 0i64;
			if (true /*sub_14131F910*/)
			{
				sub_14131F910(*(uint64_t *)(v1 + 96), g_ModuleBase + 0x34B5230);
			}
			else
			{
				v12 = *(uint64_t *)(v1 + 96);
				if (v12)
				{
					// BSGraphics::Renderer::CleanupCommandBuffer or similar name (????)
					sub_1401C49C0(*(uint64_t *)(v1 + 96), 0i64, 0i64);
					operator_delete(v12, 16i64);
				}
			}
			*(uint64_t *)(v1 + 96) = v11;
		} while (v11);
	}

	*(DWORD *)(v1 + 88) = 0;
}

void BSBatchRenderer::RenderPassGroup::Clear(bool Validate)
{
	for (int i = 0; i < ARRAYSIZE(this->m_Pass); i++)
	{
		if (Validate && this->m_Pass[i])
		{
			//sub_14002288B((__int64)&a1, 255i64, (__int64)"Pass still has passes");
			//if (qword_14468B560)
			//	qword_14468B560(&a1, 0i64);
		}

		// This is removed in public builds?
		// for (result = *(_QWORD *)(v4 + 8 * v3); result; result = *(_QWORD *)(result + 48))
		//	*(_BYTE *)(result + 33) = 0;

		this->m_Pass[i] = nullptr;
	}

	this->UnkDword1 = 0;
}