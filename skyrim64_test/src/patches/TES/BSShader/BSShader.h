#pragma once

#include <map>
#include <stdint.h>
#include <functional>
#include "../NiMain/common.h"
#include "../BSTScatterTable.h"
#include "BSShaderProperty.h"
#include "BSShaderMaterial.h"

struct BSRenderPass;
struct BSVertexShader;
struct BSPixelShader;
struct BSIStream;

#define BSSHADER_FORWARD_DEBUG 0

#define BSSHADER_FORWARD_CALL_ALWAYS(OptionIndex, Func, ...) \
{ \
	static uint32_t vtableIndex = vtable_index_util::getIndexOf(Func); \
	auto realFunc = Func; \
	*(uintptr_t *)&realFunc = *(uintptr_t*)(g_ModuleBase + OriginalVTableBase + (8 * vtableIndex)); \
	return (this->*realFunc)(__VA_ARGS__); \
}

#define BSSHADER_FORWARD_CALL(OptionIndex, Func, ...) \
if (g_ShaderToggles[m_Type][BSGraphics::CONSTANT_GROUP_LEVEL_##OptionIndex]) { BSSHADER_FORWARD_CALL_ALWAYS(OptionIndex, Func, __VA_ARGS__) }

#define DEFINE_SHADER_DESCRIPTOR(Type, Entries) static const ShaderDescriptor ShaderConfig(Type, { Entries })
#define CONFIG_ENTRY(a, b, c, d, e) { ShaderDescriptor::##a, ShaderDescriptor::##b, c, #d, #e },

class ShaderDescriptor
{
public:
	enum ShaderType
	{
		PS,			// Pixel shader
		VS,			// Vertex shader
		CS,			// Compute shader
	};

	enum DeclType
	{
		PER_TEC,	// Constants
		PER_MAT,	// Constants
		PER_GEO,	// Constants
		SAMPLER,	// Samplers
		TEXTURE,	// Textures
	};

	struct Entry
	{
		ShaderType m_ShaderType;
		DeclType m_DeclType;
		int Index;
		const char *DataType;
		const char *Name;
	};

	const char *const Type;
	std::map<int, const Entry *> ByConstantIndexVS;
	std::map<int, const Entry *> ByConstantIndexPS;
	std::map<int, const Entry *> ByConstantIndexCS;
	std::map<int, const Entry *> BySamplerIndex;
	std::map<int, const Entry *> ByTextureIndex;

private:
	std::vector<Entry> m_Entries;

public:
	ShaderDescriptor(const char *FXType, std::initializer_list<Entry> InputData) : Type(FXType), m_Entries(InputData)
	{
		for (Entry& e : m_Entries)
		{
			if (e.m_DeclType == PER_TEC || e.m_DeclType == PER_MAT || e.m_DeclType == PER_GEO)
			{
				switch (e.m_ShaderType)
				{
				case VS: ByConstantIndexVS[e.Index] = &e; break;
				case PS: ByConstantIndexPS[e.Index] = &e; break;
				case CS: ByConstantIndexCS[e.Index] = &e; break;
				default: Assert(false); break;
				}
			}

			if (e.m_DeclType == SAMPLER)
				BySamplerIndex[e.Index] = &e;

			if (e.m_DeclType == TEXTURE)
				ByTextureIndex[e.Index] = &e;
		}

		// m_Entries should never be modified after this point
	}
};

class NiBoneMatrixSetterI
{
public:
#pragma pack(push, 1)
	struct Data
	{
		char _pad0[0x3C];
		uint16_t m_Flags;
		char _pad1[0x12];

		Data()
		{
			auto sub_140C7B160 = (void(__fastcall *)(Data *))(g_ModuleBase + 0xC7B160);
			sub_140C7B160(this);
		}

		~Data()
		{
			auto sub_140C7B1A0 = (void(__fastcall *)(Data *))(g_ModuleBase + 0xC7B1A0);
			sub_140C7B1A0(this);
		}
	};
#pragma pack(pop)

	virtual ~NiBoneMatrixSetterI()
	{
		Assert(false);
	}

	virtual void SetBoneMatrix(NiSkinInstance *SkinInstance, Data *Parameters, const NiTransform *Transform) = 0;
};
static_assert(sizeof(NiBoneMatrixSetterI) == 0x8);
static_assert(sizeof(NiBoneMatrixSetterI::Data) == 0x50);
static_assert_offset(NiBoneMatrixSetterI::Data, m_Flags, 0x3C);

class BSReloadShaderI
{
public:
	virtual void ReloadShaders(BSIStream *Stream) = 0;
};

class BSShader : public NiRefObject, public NiBoneMatrixSetterI, public BSReloadShaderI
{
private:
	template<typename T>
	class TechniqueIDStorage
	{
	public:
		T m_Value;

		uint32_t GetKey()
		{
			return m_Value->m_TechniqueID;
		}
	};

	template<typename T, typename Storage = TechniqueIDStorage<T>>
	class TechniqueIDMap : public BSTScatterTable<
		uint32_t,
		T,
		Storage,
		BSTScatterTableDefaultHashPolicy<uint32_t>,
		BSTScatterTableHeapAllocator<BSTScatterTableEntry<uint32_t, T, Storage>>>
	{
	};

public:
	static bool g_ShaderToggles[16][3];

	BSShader(const char *LoaderType);
	virtual ~BSShader();

	virtual bool SetupTechnique(uint32_t Technique) = 0;
	virtual void RestoreTechnique(uint32_t Technique) = 0;
	virtual void SetupMaterial(BSShaderMaterial const *Material);
	virtual void RestoreMaterial(BSShaderMaterial const *Material);
	virtual void SetupGeometry(BSRenderPass *Pass, uint32_t Flags) = 0;
	virtual void RestoreGeometry(BSRenderPass *Pass, uint32_t RenderFlags) = 0;
	virtual void GetTechniqueName(uint32_t Technique, char *Buffer, uint32_t BufferSize);
	virtual void ReloadShaders(bool Unknown);

	virtual void ReloadShaders(BSIStream *Stream) override;
	virtual void SetBoneMatrix(NiSkinInstance *SkinInstance, Data *Parameters, const NiTransform *Transform) override;

	// Both of these functions are virtual, but removed from SkyrimSE.exe itself
	void CreateVertexShader(
		uint32_t Technique,
		const char *SourceFile,
		const std::vector<std::pair<const char *, const char *>>& Defines,
		std::function<const char *(int Index)> GetConstant);

	void CreatePixelShader(
		uint32_t Technique,
		const char *SourceFile,
		const std::vector<std::pair<const char *, const char *>>& Defines,
		std::function<const char *(int Index)> GetSampler,
		std::function<const char *(int Index)> GetConstant);

	bool BeginTechnique(uint32_t VertexShaderID, uint32_t PixelShaderID, bool IgnorePixelShader);
	void EndTechnique();

	void SetupGeometryAlphaBlending(const NiAlphaProperty *AlphaProperty, BSShaderProperty *ShaderProperty, bool a4);
	void SetupAlphaTestRef(const NiAlphaProperty *AlphaProperty, BSShaderProperty *ShaderProperty);

	uint32_t m_Type;
	TechniqueIDMap<BSVertexShader *> m_VertexShaderTable;
	TechniqueIDMap<BSPixelShader *> m_PixelShaderTable;
	const char *m_LoaderType;
};
static_assert(sizeof(BSShader) == 0x90, "");
static_assert(offsetof(BSShader, m_Type) == 0x20, "");
static_assert(offsetof(BSShader, m_VertexShaderTable) == 0x28, "");
static_assert(offsetof(BSShader, m_PixelShaderTable) == 0x58, "");
static_assert(offsetof(BSShader, m_LoaderType) == 0x88, "");

STATIC_CONSTRUCTOR(__CheckBSShaderVtable, []
{
	assert_vtable_index(&BSShader::~BSShader, 0);
	assert_vtable_index(&BSShader::DeleteThis, 1);
	assert_vtable_index(&BSShader::SetupTechnique, 2);
	assert_vtable_index(&BSShader::RestoreTechnique, 3);
	assert_vtable_index(&BSShader::SetupMaterial, 4);
	assert_vtable_index(&BSShader::RestoreMaterial, 5);
	assert_vtable_index(&BSShader::SetupGeometry, 6);
	assert_vtable_index(&BSShader::RestoreGeometry, 7);
	assert_vtable_index(&BSShader::GetTechniqueName, 8);
	assert_vtable_index(static_cast<void(BSShader::*)(bool)>(&BSShader::ReloadShaders), 9);
});