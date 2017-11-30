#include "../rendering/common.h"
#include "../../common.h"
#include "BSShader/BSShaderManager.h"
#include "MemoryContextTracker.h"
#include "BSSpinLock.h"
#include "BSBatchRenderer.h"

AutoPtr<BYTE, 0x31F54CD> byte_1431F54CD;
AutoPtr<DWORD, 0x1E32FDC> dword_141E32FDC;

unsigned __int64 *sub_141320370(__int64 a1, unsigned __int64 *a2);
signed __int64 *sub_1413203D0(__int64 a1, signed __int64 *a2);

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

__int64 sub_14131ED70(uint64_t *a1, unsigned int a2, unsigned __int8 a3, unsigned int a4)
{
	unsigned int v5; // ebp
	__int64 v6; // r14
	unsigned __int8 v7; // r15
	__int64 v8; // rsi
	uint64_t *v9; // rbx
	__int64 result; // rax
	__int64 v11; // rdi
	bool v12; // zf

	auto GraphicsGlobals = (BSGraphicsRendererGlobals *)GetThreadedGlobals();

	uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GraphicsGlobals + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GraphicsGlobals + 0x3500);

	auto sub_14131EFF0 = (__int64(__fastcall *)(unsigned int a1, __int64 a2))(g_ModuleBase + 0x131F350);
	auto sub_14131F450 = (__int64(__fastcall *)(__int64 *a1, __int64 a2, unsigned int a3))(g_ModuleBase + 0x131F7B0);
	auto sub_14131F1F0 = (__int64(__fastcall *)(__int64 *a1, char a2, unsigned int a3))(g_ModuleBase + 0x131F550);
	auto sub_14131F2A0 = (__int64(__fastcall *)(__int64 a1, unsigned __int8 a2, unsigned int a3))(g_ModuleBase + 0x131F600);

	v5 = a4;
	v6 = *a1;
	v7 = a3;
	v8 = a1[2];
	v9 = a1;
	if (dword_1432A8214 == a2 && a2 != 0x5C006076 && v6 == qword_1432A8218
		|| (dword_141E32FDC = a2, result = sub_14131EFF0(a2, v6), (BYTE)result))
	{
		v11 = v9[1];
		if (v11)
			v11 = *(uint64_t *)(v11 + 120);
		if (v11 != qword_1434B5220)
		{
			if (v11)
				(*(void(__fastcall **)(__int64, __int64))(*(uint64_t *)v6 + 32i64))(v6, v11);
			qword_1434B5220 = v11;
		}
		v12 = *(uint64_t *)(v8 + 304) == 0i64;
		*(BYTE *)(v8 + 264) = *((BYTE *)v9 + 30);

		if (v12)
		{
			if (*(BYTE *)(v8 + 265) & 8)
				result = sub_14131F450((__int64 *)v9, v7, v5);
			else
				result = sub_14131F1F0((__int64 *)v9, v7, v5); // v9 = Render pass, *(v9 + 16) = Render pass geometry, *(v9) = BSShader
		}
		else
		{
			result = sub_14131F2A0((__int64)v9, v7, v5);// Something to do with skinning, "Render Error : Skin instance is nullptr"
		}
	}
	return result;
}

void sub_14131F9F0(__int64 *a1, unsigned int a2)
{
	unsigned int v4; // ebp
	__int64 *v5; // rdi
	__int64 result; // rax
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
		if (qword_1432A8218)
			result = (*(__int64(__fastcall **)(__int64, uint64_t))(*(uint64_t *)qword_1432A8218 + 24i64))(
				qword_1432A8218,
				(unsigned int)dword_1432A8214);
		qword_1432A8218 = 0i64;
		dword_1432A8214 = 0;
		qword_1434B5220 = 0i64;
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
				if (*(uint64_t *)(*(uint64_t *)(v7 + 8) + 56i64) & 0x1000000000i64)
				{
					if (!*(DWORD *)&GraphicsGlobals->__zz0[52])
						goto LABEL_13;
					*(DWORD *)&GraphicsGlobals->__zz0[52] = 0;
				}
				else
				{
					if (*(DWORD *)&GraphicsGlobals->__zz0[52] == 1)
						goto LABEL_13;
					*(DWORD *)&GraphicsGlobals->__zz0[52] = 1;
				}
				GraphicsGlobals->dword_14304DEB0 |= 0x20u;
			}
		LABEL_13:
			v9 = *(uint64_t *)(*(uint64_t *)(v7 + 16) + 288i64);
			v10 = v9 && (*(WORD *)(v9 + 48) >> 9) & 1;
			result = sub_14131ED70((uint64_t *)v7, v8, v10, v4);
			goto LABEL_18;
		}
	LABEL_19:
		if (!(v4 & 0x108) && *(DWORD *)&GraphicsGlobals->__zz0[52] != 1)
		{
			GraphicsGlobals->dword_14304DEB0 |= 0x20u;
			*(DWORD *)&GraphicsGlobals->__zz0[52] = 1;
		}
		*v5 = 0i64;
		v5[1] = 0i64;
		if (qword_1432A8218)
			result = (*(__int64(__fastcall **)(__int64, uint64_t))(*(uint64_t *)qword_1432A8218 + 24i64))(
				qword_1432A8218,
				(unsigned int)dword_1432A8214);
		qword_1432A8218 = 0i64;
		dword_1432A8214 = 0;
		qword_1434B5220 = 0i64;
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

	signed int *v3; // r9
	signed int v4; // eax
	bool v5; // zf
	signed __int64 v6; // r8

	v3 = a3;
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
			if (*(uint64_t *)(*(uint64_t *)(a1 + 8) + 8 * (6i64 * a2 + v6)))
			{
				*v3 = v4;
				v4 = 5;
				v6 = 5i64;
			}
			++v4;
			++v6;
			v5 = v4 == 5;
		} while (v4 < 5);
	}
	if (v5)
		*v3 = 0;
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
	signed __int64 v8; // rcx

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
	{
		v8 = *(uint64_t *)(a1 + 8) + 48i64 * v6;
		*(uint64_t *)v8 = 0i64;
		*(uint64_t *)(v8 + 8) = 0i64;
		*(uint64_t *)(v8 + 16) = 0i64;
		*(uint64_t *)(v8 + 24) = 0i64;
		*(uint64_t *)(v8 + 32) = 0i64;
		*(DWORD *)(v8 + 40) = 0;
	}
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
	__int64 a1 = (__int64)this;

	__int64 v6; // r11
	unsigned int v7; // ebx
	unsigned int *v8; // r15
	unsigned int *v9; // r13
	__int64 v10; // rsi
	signed __int64 v11; // r9
	unsigned __int8 v12; // di
	signed int v13; // eax
	bool v14; // r10
	bool v15; // zf
	uint32_t v16; // r8
	char v17; // cl
	uint32_t v18; // r9
	__int64 v19; // r14
	uint64_t *v20; // rbx
	__int64 v21; // rcx
	signed __int64 v22; // rdx
	__int64 v24; // [rsp+68h] [rbp+20h]

	auto& GraphicsGlobals = *(BSGraphicsRendererGlobals *)GetThreadedGlobals();

	uint32_t& dword_1432A8210 = *(uint32_t *)((uintptr_t)GetThreadedGlobals() + 0x3010);
	uint32_t& dword_1432A8214 = *(uint32_t *)((uintptr_t)GetThreadedGlobals() + 0x3014);
	uint64_t& qword_1432A8218 = *(uint64_t *)((uintptr_t)GetThreadedGlobals() + 0x3018);
	uint64_t& qword_1434B5220 = *(uint64_t *)((uintptr_t)GetThreadedGlobals() + 0x3500);

	v24 = a4;
	v6 = *(uint64_t *)(a1 + 72);
	v7 = 0;
	v8 = a3;
	v9 = a2;
	v10 = a1;

	// BSTScatterTree
	if (v6)
	{
		v11 = v6 + 16i64 * (*a2 & (*(DWORD *)(a1 + 44) - 1));
		if (*(uint64_t *)(v11 + 8))
		{
			while (*(DWORD *)v11 != *a2)
			{
				v11 = *(uint64_t *)(v11 + 8);
				if (v11 == *(uint64_t *)(a1 + 56))
					goto LABEL_7;
			}
			v7 = *(DWORD *)(v11 + 4);
		}
	}
LABEL_7:
	v12 = 0;
	v13 = *(DWORD *)&GraphicsGlobals.__zz0[52];
	v14 = (a5 & 0x108) != 0;
	v15 = *a3 == 0;
	v16 = GraphicsGlobals.dword_14304DEB0;
	if (v15)
	{
		if (!v14 && *(DWORD *)&GraphicsGlobals.__zz0[52] != 1)
		{
			v13 = 1;
			v16 = GraphicsGlobals.dword_14304DEB0 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 1;
			GraphicsGlobals.dword_14304DEB0 |= 0x20u;
		}
		v17 = GraphicsGlobals.__zz0[76];
		if (GraphicsGlobals.__zz0[76])
		{
			v17 = 0;
			v16 = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		v18 = *(unsigned int *)&GraphicsGlobals.__zz0[68];
		if (*(DWORD *)&GraphicsGlobals.__zz0[68])
		{
			v18 = 0i64;
			v16 = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
	}
	else
	{
		v17 = GraphicsGlobals.__zz0[76];
		v18 = *(unsigned int *)&GraphicsGlobals.__zz0[68];
	}
	if (*v8 == 2)
	{
		if (!v14 && v13)
		{
			v13 = 0;
			v16 = (unsigned int)v16 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if (v17)
		{
			v17 = 0;
			v16 = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if ((DWORD)v18)
		{
			v18 = 0i64;
			v16 = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
	}
	if (*v8 == 3)
	{
		if (!v14 && v13)
		{
			v13 = 0;
			v16 = (unsigned int)v16 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if (v17 != 1)
		{
			v17 = 1;
			v16 = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		v12 = 1;
		if (byte_1431F54CD && (DWORD)v18 != 1)
		{
			v16 = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
			v18 = 1i64;
		}
	}
	if (*v8 == 1)
	{
		if (!v14 && v13 != 1)
		{
			v13 = 1;
			v16 = (unsigned int)v16 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if (v17 != 1)
		{
			v17 = 1;
			v16 = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		v12 = 1;
		if (byte_1431F54CD && (DWORD)v18 != 1)
		{
			v16 = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
			v18 = 1i64;
		}
	}
	if (*v8 == 4)
	{
		if (!v14 && v13 != 1)
		{
			v16 = (unsigned int)v16 | 0x20;
			*(DWORD *)&GraphicsGlobals.__zz0[52] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		if (v17 != 1)
		{
			v16 = v16 | 0x100;
			GraphicsGlobals.__zz0[76] = 1;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
		v12 = 1;
		if ((DWORD)v18)
		{
			v18 = 0i64;
			v16 = v16 | 0x80;
			*(DWORD *)&GraphicsGlobals.__zz0[68] = 0;
			GraphicsGlobals.dword_14304DEB0 = v16;
		}
	}
	v19 = v7;
	v20 = *(uint64_t **)(*(uint64_t *)(v10 + 8) + 8 * ((signed int)*v8 + 6i64 * v7));
	if (v20)
	{
		do
		{
			sub_14131ED70(v20, *v9, v12, a5);
			v20 = (uint64_t *)v20[6];
		} while (v20);
		v18 = *(unsigned int *)&GraphicsGlobals.__zz0[68];
		v16 = GraphicsGlobals.dword_14304DEB0;
	}
	if (*(BYTE *)(v10 + 108))
	{
		v21 = *v8;
		v22 = *(uint64_t *)(v10 + 8) + 48 * v19;
		*(DWORD *)(v22 + 40) &= ~(1 << v21);
		*(uint64_t *)(v22 + 8 * v21) = 0i64;
		v18 = *(unsigned int *)&GraphicsGlobals.__zz0[68];
		v16 = GraphicsGlobals.dword_14304DEB0;
	}
	if (qword_1432A8218)
	{
		(*(void(__fastcall **)(__int64, uint64_t, __int64, signed __int64))(*(uint64_t *)qword_1432A8218 + 24i64))(
			qword_1432A8218,
			(unsigned int)dword_1432A8214,
			v16,
			v18);
		v18 = *(DWORD *)&GraphicsGlobals.__zz0[68];
		v16 = GraphicsGlobals.dword_14304DEB0;
	}
	qword_1432A8218 = 0i64;
	dword_1432A8214 = 0;
	qword_1434B5220 = 0i64;
	if (v18)
	{
		*(DWORD *)&GraphicsGlobals.__zz0[68] = 0;
		GraphicsGlobals.dword_14304DEB0 = v16 | 0x80;
	}
	++*v8;
	return sub_14131E700((uint32_t *)v9, (__int64)v8, v24);
}

void BSBatchRenderer::sub_14131D6E0(__int64 a1)
{
	__int64 v1; // rbp
	unsigned int v4; // er15
	unsigned __int64 *v5; // rax
	unsigned __int64 v6; // rdi
	unsigned __int64 v7; // r14
	__int64 v8; // rdx
	signed __int64 v9; // rcx
	__int64 v10; // rax
	__int64 v11; // rsi
	__int64 v12; // rdi
	__int64 v13[2]; // [rsp+28h] [rbp-50h]
	char v14[16]; // [rsp+38h] [rbp-40h]

	v1 = a1;

	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	v4 = 0;
	v5 = (unsigned __int64 *)sub_141320350(a1 + 32, (__int64)&v13); // BSTScatterTable::begin()
	v6 = *v5;
	v7 = v5[1];
	v8 = *(uint64_t *)sub_1413203B0(v1 + 32, (__int64)&v14);// BSTScatterTable::end()
	while (v6 != v8)
	{
		// This code uses both BSTScatterTable and BSTArray
		// Also an inlined function. "Pass still has passes"
		if (v6)
			v4 = *(DWORD *)(v6 + 4);
		v9 = *(uint64_t *)(v1 + 8) + 48i64 * v4;
		*(uint64_t *)v9 = 0i64;
		*(uint64_t *)(v9 + 8) = 0i64;
		*(uint64_t *)(v9 + 16) = 0i64;
		*(uint64_t *)(v9 + 24) = 0i64;
		*(uint64_t *)(v9 + 32) = 0i64;
		*(DWORD *)(v9 + 40) = 0;
		do
			v6 += 16i64;
		while (v6 < v7 && !*(uint64_t *)(v6 + 8));
	}

	// This entire block is some inlined function
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