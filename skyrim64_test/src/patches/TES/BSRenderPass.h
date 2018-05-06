#pragma once

#include "BSShader/BSShader.h"

struct BSRenderPass
{
	const static int MaxLightInArrayC = 16;

	BSShader *m_Shader;
	BSShaderProperty *m_Property;
	BSGeometry *m_Geometry;
	uint32_t m_TechniqueID;
	uint8_t Byte1C;
	uint8_t Byte1D;
	uint8_t Byte1E;
	uint8_t m_LightCount;
	uint16_t Word20;
	void *UnkPtr28;			// Possibly previous sub-pass
	BSRenderPass *m_Next;	// Possibly next sub-pass
	void **m_SceneLights;	// Pointer to an array of 16 lights (MaxLightInArrayC, directional only?, restricted to 3?)
	uint32_t UnkDword40;	// Set from TLS variable. Index in BSRenderPassCache?

	/*
	This might be an assignment ctor

	BSRenderPass(BSShader *Shader, BSShaderProperty *Property, BSGeometry *Geometry, uint32_t TechniqueID, uint8_t NewLightCount, void **NewSceneLights)
	{
		m_Shader = Shader;
		m_Property = Property;
		m_Geometry = Geometry;
		m_TechniqueID = TechniqueID;
		Byte1C = 0;
		Byte1D = 0;
		Byte1E = 3;
		m_LightCount = NewLightCount;
		Word20 = 0;
		UnkPtr28 = nullptr;
		m_Next = nullptr;
		UnkDword40 = 0;

		Assert(m_SceneLights);

		SetLights(NewLightCount, NewSceneLights);
	}
	*/

	void SetLights(uint8_t SceneLightCount, void **SceneLights)
	{
		Assert(SceneLightCount <= MaxLightInArrayC, "MaxLightInArrayC is too small");

		m_LightCount = SceneLightCount;

		for (int i = 0; i < SceneLightCount; i++)
			m_SceneLights[i] = SceneLights[i];

		// Zero the remainder
		for (int i = SceneLightCount; i < MaxLightInArrayC; i++)
			m_SceneLights[i] = nullptr;

	}

	NiAlphaProperty *QAlphaProperty() const
	{
		return m_Geometry->QAlphaProperty();
	}

	void **QLights() const
	{
		return m_SceneLights;
	}
};
static_assert(sizeof(BSRenderPass) == 0x48);
static_assert(offsetof(BSRenderPass, m_Shader) == 0x0, "");
static_assert(offsetof(BSRenderPass, m_Property) == 0x8, "");
static_assert(offsetof(BSRenderPass, m_Geometry) == 0x10, "");
static_assert(offsetof(BSRenderPass, m_TechniqueID) == 0x18, "");
static_assert(offsetof(BSRenderPass, Byte1C) == 0x1C, "");
static_assert(offsetof(BSRenderPass, Byte1D) == 0x1D, "");
static_assert(offsetof(BSRenderPass, Byte1E) == 0x1E, "");
static_assert(offsetof(BSRenderPass, m_LightCount) == 0x1F, "");
static_assert(offsetof(BSRenderPass, Word20) == 0x20, "");
static_assert(offsetof(BSRenderPass, UnkPtr28) == 0x28, "");
static_assert(offsetof(BSRenderPass, m_Next) == 0x30, "");
static_assert(offsetof(BSRenderPass, m_SceneLights) == 0x38, "");
static_assert(offsetof(BSRenderPass, UnkDword40) == 0x40, "");