#include "../../../common.h"
#include "BSShaderProperty.h"

const uint32_t BSShaderProperty::UniqueMaterialFlags[15] =
{
	0x07, 0x26, 0x0B, 0x0A, 0x15, 0x12, 0x1C,
	0x0E, 0x21, 0x38, 0x3D, 0x22, 0x29, 0x3F,
	0x2E
};

const char *BSShaderProperty::UniqueMaterialNames[15] =
{
	"Envmap",
	"Glowmap",
	"Parallax",
	"Facegen",
	"Facegen RGB Tint",
	"Hair Tint",
	"Parallax Occlusion",
	"Multitexture Landscape",
	"LOD Landscape",
	"Multilayer Parallax",
	"Tree Anim",
	"LOB Objects",
	"MultiIndex",
	"LOD Objects HD",
	"Multitexture Landscape (LOD Blend)"
};

const char *BSShaderProperty::MaterialBitNames[64] =
{
	"specular",
	"skinned",
	"temp refraction",
	"vertex alpha",
	"grayscale to palette - color",
	"grayscale to palette - alpha",
	"falloff",
	"environment mapping",
	"receive shadows",
	"cast shadows",
	"facegen",
	"parallax",
	"model-space normals",
	"non-projective shadows",
	"landscape",
	"refraction",
	"fire refraction",
	"eye environment mapping",
	"hair",
	"screendoor alpha fade",
	"localmap hide secret",
	"facegen rgb tint",
	"own emit",
	"projected uv",
	"multiple textures",
	"remappable textures",
	"decal",
	"dynamic decal",
	"parallax occlusion",
	"external emittance",
	"soft effect",
	"zbuffer test",
	"zbuffer write",
	"lod landscape",
	"lod objects",
	"no fade",
	"two-sided geometry",
	"vertex colors",
	"glow map",
	"assume shadowmask",
	"character lighting",
	"multi index snow",
	"vertex lighting",
	"uniform scale",
	"fit slope",
	"billboard",
	"no lod land blend",
	"envmap light fade",
	"wireframe",
	"weapon blood",
	"hide on local map",
	"premult alpha",
	"cloud LOD",
	"anisotropic lighting",
	"no transparency multisampling",
	"unused01",
	"multi layer parallax",
	"soft lighting",
	"rim lighting",
	"back lighting",
	"snow",
	"tree anim",
	"effect lighting",
	"hd lod objects"
};

void BSShaderProperty::GetMaterialString(uint64_t Flags, char *Buffer, size_t BufferSize)
{
	// Uniques - can only have 1 of these bits set at once
	strcpy_s(Buffer, BufferSize, "Unique flags: [");

	for (int i = 0; i < ARRAYSIZE(UniqueMaterialFlags); i++)
	{
		uint32_t bit = UniqueMaterialFlags[i];

		Assert(bit < 64);

		if (Flags & (1ull << bit))
		{
			strcat_s(Buffer, BufferSize, UniqueMaterialNames[i]);
			strcat_s(Buffer, BufferSize, ", ");
		}

		Trim(Buffer, ' ');
		Trim(Buffer, ',');
	}

	// Everything - each bit corresponds to an array index
	strcat_s(Buffer, BufferSize, "] Flags: [");

	for (int i = 0; i < ARRAYSIZE(MaterialBitNames); i++)
	{
		uint32_t bit = i;

		Assert(bit < 64);

		if (Flags & (1ull << bit))
		{
			strcat_s(Buffer, BufferSize, MaterialBitNames[i]);
			strcat_s(Buffer, BufferSize, ", ");
		}

		Trim(Buffer, ' ');
		Trim(Buffer, ',');
	}

	strcat_s(Buffer, BufferSize, "]");
}