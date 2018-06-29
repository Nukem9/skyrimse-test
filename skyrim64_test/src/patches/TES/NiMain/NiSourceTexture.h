#pragma once

#include "NiTexture.h"
#include "../BSGraphicsRenderer.h"

class NiSourceTexture : public NiTexture
{
public:
	NiSourceTexture();
	virtual ~NiSourceTexture();

	char _pad0[0x8];// BSResource::Stream *
	BSGraphics::Texture *pRendererTexture;
	char _pad1[0x8];// uint8_t flags

	BSGraphics::Texture *QRendererTexture() const
	{
		return pRendererTexture;
	}
};
static_assert(sizeof(NiSourceTexture) == 0x58);
static_assert_offset(NiSourceTexture, pRendererTexture, 0x48);