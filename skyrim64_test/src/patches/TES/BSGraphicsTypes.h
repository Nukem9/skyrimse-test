#pragma once

#define MAX_SHARED_PARTICLES_SIZE 2048
#define MAX_PARTICLE_STRIP_COUNT 51200

#define MAX_VS_CONSTANTS 20
#define MAX_PS_CONSTANTS 64

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

	struct Buffer
	{
		ID3D11Buffer *m_Buffer;	// Selected from pool in Load*ShaderFromFile()
		void *m_Data;			// m_Data = DeviceContext->Map(m_Buffer)

		// Based on shader load flags, these **can be null**. At least one of the
		// pointers is guaranteed to be non-null.
	};
	static_assert_offset(Buffer, m_Buffer, 0x0);
	static_assert_offset(Buffer, m_Data, 0x8);

	//
	// Constant group types used for shader parameters
	//
	const uint8_t INVALID_CONSTANT_BUFFER_OFFSET = 0xFF;

	enum ConstantGroupLevel
	{
		CONSTANT_GROUP_LEVEL_TECHNIQUE = 0x0,		// Varies between PS/VS shaders
		CONSTANT_GROUP_LEVEL_MATERIAL = 0x1,		// Varies between PS/VS shaders
		CONSTANT_GROUP_LEVEL_GEOMETRY = 0x2,		// Varies between PS/VS shaders
		CONSTANT_GROUP_LEVEL_COUNT = 0x3,

													// Slot 7 is used for grass but unknown
		CONSTANT_GROUP_LEVEL_INSTANCE = 0x8,		// Instanced geometry such as grass and trees
		CONSTANT_GROUP_LEVEL_PREVIOUS_BONES = 0x9,
		CONSTANT_GROUP_LEVEL_BONES = 0xA,
		CONSTANT_GROUP_LEVEL_ALPHA_TEST_REF = 0xB,	// PS/VS. Used as a single float value for alpha testing (16 bytes allocated)
		CONSTANT_GROUP_LEVEL_PERFRAME = 0xC,		// PS/VS. Per-frame constants. Contains view projections and some other variables.
	};

	class CustomConstantGroup
	{
	protected:
		//
		// Invalid constant offsets still need a place to be written to. This is supposed to
		// be in ConstantGroup<T>, but it causes a compiler crash.
		//
		// See: ConstantGroup<T>::ParamVS, INVALID_CONSTANT_BUFFER_OFFSET
		//
		inline static char EmptyWriteBuffer[1024];

	public:
		D3D11_MAPPED_SUBRESOURCE m_Map {};
		ID3D11Buffer *m_Buffer			= nullptr;
		bool m_Unified					= false;	// True if buffer is from global ring buffer
		uint32_t m_UnifiedByteOffset	= 0;		// Offset into ring buffer

	public:
		inline void *RawData() const
		{
			return m_Map.pData;
		}
	};

	template<typename T>
	class ConstantGroup : public CustomConstantGroup
	{
	public:
		T *m_Shader;

	private:
		template<typename U>
		U& MapVar(uint32_t Offset) const
		{
			static_assert(sizeof(U) <= sizeof(EmptyWriteBuffer));

			if (RawData() == nullptr || Offset == INVALID_CONSTANT_BUFFER_OFFSET)
				return *(U *)EmptyWriteBuffer;

			return *(U *)((uintptr_t)RawData() + (Offset * sizeof(float)));
		}

	public:
		template<typename U, uint32_t ParamIndex>
		U& ParamVS() const
		{
			static_assert(std::is_same<T, VertexShader>::value, "ParamVS() requires ConstantGroup<VertexShader>");
			static_assert(ParamIndex < MAX_VS_CONSTANTS);

			return MapVar<U>(m_Shader->m_ConstantOffsets[ParamIndex]);
		}

		template<typename U, uint32_t ParamIndex>
		U& ParamPS() const
		{
			static_assert(std::is_same<T, PixelShader>::value, "ParamPS() requires ConstantGroup<PixelShader>");
			static_assert(ParamIndex < MAX_PS_CONSTANTS);

			return MapVar<U>(m_Shader->m_ConstantOffsets[ParamIndex]);
		}
	};

	//
	// Shaders (TODO: Move from .h files to here)
	//
#pragma warning(push)
#pragma warning(disable:4200)
	struct VertexShader
	{
		uint32_t m_TechniqueID;						// Bit flags
		ID3D11VertexShader *m_Shader;				// DirectX handle
		uint32_t m_ShaderLength;					// Raw bytecode length

		union
		{
			struct
			{
				Buffer m_PerTechnique;				// CONSTANT_GROUP_LEVEL_TECHNIQUE
				Buffer m_PerMaterial;				// CONSTANT_GROUP_LEVEL_MATERIAL
				Buffer m_PerGeometry;				// CONSTANT_GROUP_LEVEL_GEOMETRY
			};

			Buffer m_ConstantGroups[CONSTANT_GROUP_LEVEL_COUNT];
		};

		uint64_t m_VertexDescription;				// ID3D11Device::CreateInputLayout lookup (for VSMain)
		uint8_t m_ConstantOffsets[MAX_VS_CONSTANTS];// Actual offset is multiplied by 4
		uint8_t __padding[4];
		uint8_t m_RawBytecode[0];					// Raw bytecode
	};
#pragma warning(pop)
	static_assert_offset(VertexShader, m_TechniqueID, 0x0);
	static_assert_offset(VertexShader, m_Shader, 0x8);
	static_assert_offset(VertexShader, m_ShaderLength, 0x10);
	static_assert_offset(VertexShader, m_PerTechnique, 0x18);
	static_assert_offset(VertexShader, m_PerMaterial, 0x28);
	static_assert_offset(VertexShader, m_PerGeometry, 0x38);
	static_assert_offset(VertexShader, m_VertexDescription, 0x48);
	static_assert_offset(VertexShader, m_ConstantOffsets, 0x50);
	static_assert_offset(VertexShader, m_RawBytecode, 0x68);
	static_assert(sizeof(VertexShader) == 0x68);

	struct PixelShader
	{
		uint32_t m_TechniqueID;						// Bit flags
		ID3D11PixelShader *m_Shader;				// DirectX handle

		union
		{
			struct
			{
				Buffer m_PerTechnique;				// CONSTANT_GROUP_LEVEL_TECHNIQUE
				Buffer m_PerMaterial;				// CONSTANT_GROUP_LEVEL_MATERIAL
				Buffer m_PerGeometry;				// CONSTANT_GROUP_LEVEL_GEOMETRY
			};

			Buffer m_ConstantGroups[CONSTANT_GROUP_LEVEL_COUNT];
		};

		uint8_t m_ConstantOffsets[MAX_PS_CONSTANTS];// Actual offset is multiplied by 4
	};
	static_assert_offset(PixelShader, m_TechniqueID, 0x0);
	static_assert_offset(PixelShader, m_Shader, 0x8);
	static_assert_offset(PixelShader, m_PerTechnique, 0x10);
	static_assert_offset(PixelShader, m_PerMaterial, 0x20);
	static_assert_offset(PixelShader, m_PerGeometry, 0x30);
	static_assert_offset(PixelShader, m_ConstantOffsets, 0x40);
	static_assert(sizeof(PixelShader) == 0x80);

	struct ComputeShader
	{
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