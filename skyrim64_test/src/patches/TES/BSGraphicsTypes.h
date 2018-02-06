#pragma once

#define MAX_SHARED_PARTICLES_SIZE 2048
#define MAX_PARTICLE_STRIP_COUNT 51200

namespace BSGraphics
{
	//
	// General resources
	//
	struct Texture
	{
		char _pad0[0x10];
		ID3D11ShaderResourceView *m_D3DTexture;
	};
	static_assert_offset(Texture, m_D3DTexture, 0x10);

	//
	// Shaders (TODO: Move from .h files to here)
	//

	//
	// Constant group types used for shader parameters
	//
	enum ConstantGroupLevel
	{
		CONSTANT_GROUP_LEVEL_TECHNIQUE = 0x0,		// Varies between PS/VS shaders
		CONSTANT_GROUP_LEVEL_MATERIAL = 0x1,		// Varies between PS/VS shaders
		CONSTANT_GROUP_LEVEL_GEOMETRY = 0x2,		// Varies between PS/VS shaders
		CONSTANT_GROUP_LEVEL_COUNT = 0x3,

		CONSTANT_GROUP_LEVEL_INSTANCE = 0x8,		// Instanced geometry such as grass and trees
		CONSTANT_GROUP_LEVEL_PREVIOUS_BONES = 0x9,
		CONSTANT_GROUP_LEVEL_BONES = 0xA,
		CONSTANT_GROUP_LEVEL_SCRAP_VALUE = 0xB,		// PS/VS. Used for a single float value as scrap/temp data (16 bytes allocated)
		CONSTANT_GROUP_LEVEL_SCREENSPACEINFO = 0xC,	// PS/VS. Maybe screen space/resolution info. Contains a lot of matrices and vectors.
	};

	class CustomConstantGroup
	{
	public:
		D3D11_MAPPED_SUBRESOURCE m_Map;
		ID3D11Buffer *m_Buffer;

		uint32_t m_UnifiedByteOffset;	// Offset into ringbuffer
		bool m_Unified;					// True if buffer is from global ringbuffer

	public:
		CustomConstantGroup()
		{
			memset(&m_Map, 0, sizeof(m_Map));
			m_Buffer = nullptr;
			m_UnifiedByteOffset = 0;
			m_Unified = false;
		}

		void *RawData() const
		{
			return m_Map.pData;
		}
	};

	template<typename T>
	class ConstantGroup : public CustomConstantGroup
	{
	public:
		T * m_Shader;

	public:
		template<typename U, uint32_t ParamIndex>
		U& ParamVS() const
		{
			static_assert(std::is_same<T, BSVertexShader>::value, "ParamVS() requires ConstantGroup<BSVertexShader>");
			static_assert(ParamIndex < ARRAYSIZE(T::m_ConstantOffsets));

			uintptr_t data = (uintptr_t)RawData();
			uintptr_t offset = m_Shader->m_ConstantOffsets[ParamIndex] * sizeof(float);

			return *(U *)(data + offset);
		}

		template<typename U, uint32_t ParamIndex>
		U& ParamPS() const
		{
			static_assert(std::is_same<T, BSPixelShader>::value, "ParamPS() requires ConstantGroup<BSPixelShader>");
			static_assert(ParamIndex < ARRAYSIZE(T::m_ConstantOffsets));

			uintptr_t data = (uintptr_t)RawData();
			uintptr_t offset = m_Shader->m_ConstantOffsets[ParamIndex] * sizeof(float);

			return *(U *)(data + offset);
		}
	};

	//
	// Renderer-specific types to handle uploading raw data to the GPU
	//
	struct LineShape
	{
		ID3D11Buffer *m_VertexBuffer;
		ID3D11Buffer *m_IndexBuffer;
		uint64_t m_VertexDesc;
	};
	static_assert_offset(LineShape, m_VertexBuffer, 0x0);
	static_assert_offset(LineShape, m_IndexBuffer, 0x8);
	static_assert_offset(LineShape, m_VertexDesc, 0x10);

	struct TriShape : LineShape
	{
	};

	struct DynamicTriShape
	{
		ID3D11Buffer *m_VertexBuffer;
		ID3D11Buffer *m_IndexBuffer;
		uint64_t m_VertexDesc;
		uint32_t m_VertexAllocationOffset;
		uint32_t m_VertexAllocationSize;
		uint32_t m_Unknown3;
		void *m_Unknown4;
		void *m_Unknown5;
	};
	static_assert_offset(DynamicTriShape, m_VertexBuffer, 0x0);
	static_assert_offset(DynamicTriShape, m_IndexBuffer, 0x8);
	static_assert_offset(DynamicTriShape, m_VertexDesc, 0x10);
	static_assert_offset(DynamicTriShape, m_VertexAllocationOffset, 0x18);
	static_assert_offset(DynamicTriShape, m_VertexAllocationSize, 0x1C);
	static_assert_offset(DynamicTriShape, m_Unknown3, 0x20);
	static_assert_offset(DynamicTriShape, m_Unknown4, 0x28);
	static_assert_offset(DynamicTriShape, m_Unknown5, 0x30);

	struct DynamicTriShapeDrawData : TriShape
	{
	};
}