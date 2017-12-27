#pragma once

#include "NiObjectNET.h"

class NiTexture : public NiObjectNET
{
public:
	NiTexture();
	virtual ~NiTexture();

	ID3D11ShaderResourceView *QRendererTexture()
	{
		return *(ID3D11ShaderResourceView **)((uintptr_t)this + 16);
	}
};