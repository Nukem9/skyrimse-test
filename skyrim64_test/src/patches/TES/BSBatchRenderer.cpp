#include "../rendering/common.h"
#include "../../common.h"
#include "BSGraphicsRenderer.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderAccumulator.h"
#include "MemoryContextTracker.h"
#include "BSSpinLock.h"
#include "BSBatchRenderer.h"
#include "BSReadWriteLock.h"

AutoPtr(BYTE, byte_1431F54CD, 0x31F54CD);
AutoPtr(DWORD, dword_141E32FDC, 0x1E32FDC);

unsigned __int64 *sub_141320370(__int64 a1, unsigned __int64 *a2);
signed __int64 *sub_1413203D0(__int64 a1, signed __int64 *a2);

extern BSReadWriteLock testLocks[32];

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

extern ID3DUserDefinedAnnotation *annotation;

std::wstring wide(const std::string &s)
{
	size_t srcLen = s.length();

	if (srcLen <= 0)
		return L"";

	std::wstring ret;
	ret.resize(srcLen);

	if (!MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)srcLen, ret.data(), (int)ret.size()))
		throw "Error converting UTF8 to UTF16";

	return ret;
}

uint32_t lastTechId;

SRWLOCK testingLock = SRWLOCK_INIT;

void sub_14131ED70(BSRenderPass *a1, uint32_t Technique, unsigned __int8 a3, unsigned int a4)
{
	if (InsertRenderCommand<DrawGeometryRenderCommand>(a1, Technique, a3, a4))
		return;

	//AcquireSRWLockExclusive(&testingLock);

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

	auto sub_1412ABF00 = (const char *(__fastcall *)(unsigned int a1))(g_ModuleBase + 0x12ABF00);

	v6 = (uint64_t)a1->m_Shader;

	//lastTechId = Technique;

	//char buffer[128];
	//sprintf_s(buffer, "Technique %s -- %X", sub_1412ABF00(Technique), Technique);
	//annotation->SetMarker(wide(buffer).c_str());

	if (dword_1432A8214 == Technique && Technique != 0x5C006076 && v6 == qword_1432A8218
		|| (dword_141E32FDC = Technique, result = sub_14131EFF0(Technique, v6), (BYTE)result))
	{
		BSShaderProperty *property = a1->m_Property;

		if (property)
			v11 = *(uint64_t *)((uint64_t)property + 120);// unknownFlag2 = BSShaderProperty ---- (unknownFlag2 + 120) = BSShaderMaterial *
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

		*(BYTE *)((uintptr_t)a1->m_Geometry + 264) = a1->Byte1E;

		if (*(uint64_t *)((uintptr_t)a1->m_Geometry + 304))// v8 + 304 == BSGeometry::QSkinPartitions(). NiSkinPartition?
		{
			sub_14131F2A0(a1, a3, a4);// Something to do with skinning, "Render Error : Skin instance is nullptr"
		}
		else if (*(BYTE *)((uintptr_t)a1->m_Geometry + 265) & 8)// BSGeometry::NeedsCustomRender()?
		{
			sub_14131F450(a1, a3, a4);
		}
		else
		{
			sub_14131F1F0(a1, a3, a4); // v9 = Render pass, *(v9 + 16) = Render pass geometry, *(v9) = BSShader
		}

	}

	//ReleaseSRWLockExclusive(&testingLock);
}

void sub_14131F9F0(__int64 *a1, unsigned int a2)
{
	__int64 *v5; // rdi

	v5 = a1;
	if (*a1)
	{
		sub_14131F090(true);

		BSRenderPass *i = (BSRenderPass *)*v5;

		int tempIndex = -1;
		if (i)
			tempIndex = i->m_Shader->m_Type;

		if (tempIndex != -1)
		{
			if (!InsertRenderCommand<LockShaderTypeRenderCommand>(tempIndex, true))
				testLocks[tempIndex].AcquireWrite();
				//AcquireSRWLockExclusive(&testLocks[tempIndex]);
		}

		int firstType = -1;
		for (; i; i = i->m_Next)
		{
			if (!i->m_Geometry)
				continue;

			if (firstType == -1)
				firstType = i->m_Shader->m_Type;

			if (firstType != i->m_Shader->m_Type)
				__debugbreak();

			if ((a2 & 0x108) == 0)
			{
				if (*(uint64_t *)((uint64_t)i->m_Property + 56i64) & 0x1000000000i64)// BSShaderProperty::VertexDesc?
					BSGraphics::Renderer::RasterStateSetCullMode(0);
				else
					BSGraphics::Renderer::RasterStateSetCullMode(1);
			}

			__int64 v9 = *(uint64_t *)((uintptr_t)i->m_Geometry + 288i64);// BSGeometry::GetModelBound?
			bool v10 = v9 && (*(WORD *)(v9 + 48) >> 9) & 1;
			sub_14131ED70(i, i->Dword18, v10, a2);
		}

		if ((a2 & 0x108) == 0)
			BSGraphics::Renderer::RasterStateSetCullMode(1);

		v5[0] = 0i64;
		v5[1] = 0i64;

		sub_14131F090();

		if (tempIndex != -1)
		{
			if (!InsertRenderCommand<LockShaderTypeRenderCommand>(tempIndex, false))
				testLocks[tempIndex].ReleaseWrite();
				//ReleaseSRWLockExclusive(&testLocks[tempIndex]);
		}
	}
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

		__debugbreak();

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

	if (m_BatchRenderer)
		m_BatchRenderer->sub_14131D6E0();

	this->UnkWord1 = 0;
}

bool BSBatchRenderer::HasTechniquePasses(uint32_t StartTech, uint32_t EndTech)
{
	auto sub_14131F100 = (char(__fastcall *)(__int64 a1, unsigned int a2, unsigned int a3))(g_ModuleBase + 0x131F460);
	return sub_14131F100((__int64)this, StartTech, EndTech);
}

bool BSBatchRenderer::sub_14131E8F0(unsigned int a2, uint32_t& SubPassIndex)
{
	__int64 a1 = (__int64)this;

	AssertIsRTTIType(a1, RTTI_BSBatchRenderer);

	signed int v4; // eax
	bool v5; // zf
	signed __int64 v6; // r8

	if (SubPassIndex > 4)
		SubPassIndex = 0;
	v4 = SubPassIndex;
	v5 = SubPassIndex == 5;
	if (SubPassIndex < 5)
	{
		v6 = SubPassIndex;
		do
		{
			// BSTArray
			if (this->m_RenderArrays[a2].m_Pass[v6])
			{
				SubPassIndex = v4;
				v4 = 5;
				v6 = 5i64;
			}
			++v4;
			++v6;
			v5 = v4 == 5;
		} while (v4 < 5);
	}
	if (v5)
		SubPassIndex = 0;
	return v4 == 6;
}

bool BSBatchRenderer::sub_14131E700(uint32_t& Technique, uint32_t& SubPassIndex, __int64 a4)
{
	if (!Technique)
		return sub_14131E7B0(Technique, SubPassIndex, (__int64 *)a4);

	uint32_t passArray = m_TechToArrayMap.get(Technique);

	if (!sub_14131E8F0(passArray, SubPassIndex))
		return sub_14131E7B0(Technique, SubPassIndex, (__int64 *)a4);

	return true;
}

bool BSBatchRenderer::sub_14131ECE0(uint32_t& Technique, uint32_t& SubPassIndex, __int64 a4)
{
	__int64 a1 = (__int64)this;
	uint32_t passArray = m_TechToArrayMap.get(Technique);

	if (*(BYTE *)(a1 + 108))
		m_RenderArrays[passArray].Clear(true);

	return sub_14131E700(Technique, SubPassIndex, a4);
}

bool BSBatchRenderer::sub_14131E7B0(uint32_t& Technique, uint32_t& SubPassIndex, __int64 *a4)
{
	__int64 v6; // rbx
	__int64 v7; // rax
	unsigned int v8; // ecx
	__int64 v10; // rdi

	__int64 a1 = (__int64)this;

	v6 = a1;
	v7 = *a4;

	if (!*a4 || !*(uint64_t *)(v7 + 8) && !*(DWORD *)v7)
		return false;

	v8 = *(DWORD *)v7;
	for (Technique = *(DWORD *)v7; v8 < *(DWORD *)(v6 + 80); Technique = *(DWORD *)v7)
	{
		v7 = *(uint64_t *)(v7 + 8);
		if (!v7)
			return false;
		v8 = *(DWORD *)v7;
	}

	if (v8 > *(DWORD *)(v6 + 84))
		return false;

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

	return sub_14131E8F0(m_TechToArrayMap.get(Technique), SubPassIndex);
}

bool BSBatchRenderer::sub_14131E960(uint32_t& Technique, uint32_t& SubPassIndex, __int64 a4, unsigned int a5)
{
	__int64 a1 = (__int64)this;
	AssertIsRTTIType(a1, RTTI_BSBatchRenderer);

	auto& GraphicsGlobals = *(BSGraphicsRendererGlobals *)GetThreadedGlobals();

	bool unknownFlag2 = false;
	bool unknownFlag = false;

	// Set pass render state
	{
		unknownFlag2 = 0;
		unknownFlag = (a5 & 0x108) != 0;

		int cullMode = -1;
		int alphaBlendUnknown = -1;
		bool useScrapConstant = false;

		switch (SubPassIndex)
		{
		case 0:
			if (!unknownFlag)
				cullMode = 1;

			useScrapConstant = false;
			alphaBlendUnknown = 0;
			break;

		case 1:
			if (!unknownFlag)
				cullMode = 1;

			useScrapConstant = true;
			unknownFlag2 = 1;

			if (byte_1431F54CD)
				alphaBlendUnknown = 1;
			break;

		case 2:
			if (!unknownFlag)
				cullMode = 0;

			useScrapConstant = false;
			alphaBlendUnknown = 0;
			break;

		case 3:
			if (!unknownFlag)
				cullMode = 0;

			useScrapConstant = true;
			unknownFlag2 = 1;

			if (byte_1431F54CD)
				alphaBlendUnknown = 1;
			break;

		case 4:
			if (!unknownFlag)
				cullMode = 1;

			useScrapConstant = true;
			unknownFlag2 = 1;
			alphaBlendUnknown = 0;
			break;
		}

		if (cullMode != -1)
			BSGraphics::Renderer::RasterStateSetCullMode(cullMode);

		if (alphaBlendUnknown != -1)
			BSGraphics::Renderer::AlphaBlendStateSetUnknown1(alphaBlendUnknown);

		BSGraphics::Renderer::SetUseScrapConstantValue(useScrapConstant);
	}

	// Render this group with a specific render pass list
	RenderPassArray *passArray = &m_RenderArrays[m_TechToArrayMap.get(Technique)];

	int tempIndex = -1;
	if (passArray->m_Pass[SubPassIndex])
		tempIndex = passArray->m_Pass[SubPassIndex]->m_Shader->m_Type;

	if (tempIndex != -1)
	{
		if (!InsertRenderCommand<LockShaderTypeRenderCommand>(tempIndex, true))
			testLocks[tempIndex].AcquireWrite();
			//AcquireSRWLockExclusive(&testLocks[tempIndex]);
	}

	int firstType = -1;
	for (BSRenderPass *pass = passArray->m_Pass[SubPassIndex]; pass; pass = pass->m_Next)
	{
		if (firstType == -1)
			firstType = pass->m_Shader->m_Type;

		if (pass->m_Shader->m_Type != firstType)
			__debugbreak();

		sub_14131ED70(pass, Technique, unknownFlag2, a5);
	}

	// a1+108 is probably a "remove list" flag after it's rendered, but the memory is not freed yet
	if (*(BYTE *)(a1 + 108))
	{
		if (SubPassIndex < 0 || SubPassIndex >= ARRAYSIZE(passArray->m_Pass))
			__debugbreak();

		passArray->m_PassIndexBits &= ~(1 << SubPassIndex);
		passArray->m_Pass[SubPassIndex] = nullptr;
	}

	sub_14131F090();
	BSGraphics::Renderer::AlphaBlendStateSetUnknown1(0);

	if (tempIndex != -1)
	{
		if (!InsertRenderCommand<LockShaderTypeRenderCommand>(tempIndex, false))
			testLocks[tempIndex].ReleaseWrite();
			//ReleaseSRWLockExclusive(&testLocks[tempIndex]);
	}

	SubPassIndex++;
	return sub_14131E700(Technique, SubPassIndex, a4);
}

void BSBatchRenderer::sub_14131D6E0()
{
	__int64 v1 = (__int64)this;
	AssertIsRTTIType(v1, RTTI_BSBatchRenderer);

	MemoryContextTracker tracker(32, "BSBatchRenderer.cpp");

	for (auto itr = m_TechToArrayMap.begin(); itr != m_TechToArrayMap.end(); itr++)
	{
		if (itr)
			m_RenderArrays[*itr].Clear(true);
	}

	// This entire block is some inlined function
	if (*(uint64_t *)(v1 + 96))
	{
		uint64_t v11;

		do
		{
			uint64_t v10 = *(uint64_t *)(v1 + 96);
			v11 = *(uint64_t *)(v10 + 8);
			*(uint64_t *)(v10 + 8) = 0i64;

			if (true /*sub_14131F910*/)
			{
				sub_14131F910(*(uint64_t *)(v1 + 96), g_ModuleBase + 0x34B5230);
			}
			else
			{
				uint64_t v12 = *(uint64_t *)(v1 + 96);
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

void BSBatchRenderer::RenderPassArray::Clear(bool Validate)
{
	for (int i = 0; i < ARRAYSIZE(this->m_Pass); i++)
	{
		if (Validate && this->m_Pass[i])
		{
			//sub_14002288B((__int64)&a1, 255i64, (__int64)"Pass still has passes");
			//if (qword_14468B560)
			//	qword_14468B560(&a1, 0i64);
		}

		// This is removed in public builds? Sets the bool to indicate pass is no longer registered
		// for (result = *(_QWORD *)(v4 + 8 * v3); result; result = *(_QWORD *)(result + 48))
		//	*(_BYTE *)(result + 33) = 0;

		this->m_Pass[i] = nullptr;
	}

	this->m_PassIndexBits = 0;
}