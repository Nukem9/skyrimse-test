#pragma once

class BSShaderProperty
{
private:
	static const uint32_t UniqueMaterialFlags[15];
	static const char *UniqueMaterialNames[15];
	static const char *MaterialBitNames[64];

public:
	static void GetMaterialString(uint64_t Flags, char *Buffer, size_t BufferSize);
};