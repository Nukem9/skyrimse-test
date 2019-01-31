#pragma once

#include <DirectXMath.h>
#include "NiMain/NiPointer.h"
#include "NiMain/NiSourceTexture.h"
#include "NiMain/NiPoint.h"
#include "BSTArray.h"

class NiCamera;

namespace BSGraphics
{
	struct CameraStateData
	{
		NiCamera *pReferenceCamera;
		char _pad0[0x8];
		DirectX::XMVECTOR kViewUp;
		DirectX::XMVECTOR kViewRight;
		DirectX::XMVECTOR kViewDir;
		DirectX::XMMATRIX kViewMat;
		DirectX::XMMATRIX kProjMat;
		DirectX::XMMATRIX kViewProjMat;
		char _pad1[0x40];
		DirectX::XMMATRIX kViewProjMatUnjittered;
		char _pad2[0x40];
		DirectX::XMMATRIX kProjMatUnjittered;
		char _pad3[0x60];
		NiPoint3 PosAdjust;
		char _pad4[0xC];
		NiPoint3 PreviousPosAdjust;
		bool UseJitter;
		char _pad5[0x8];

		CameraStateData();
	};
	static_assert(sizeof(CameraStateData) == 0x290);
	static_assert_offset(CameraStateData, pReferenceCamera, 0x0);
	static_assert_offset(CameraStateData, kViewUp, 0x10);
	static_assert_offset(CameraStateData, kViewRight, 0x20);
	static_assert_offset(CameraStateData, kViewDir, 0x30);
	static_assert_offset(CameraStateData, kViewMat, 0x40);
	static_assert_offset(CameraStateData, kProjMat, 0x80);
	static_assert_offset(CameraStateData, kViewProjMat, 0xC0);
	static_assert_offset(CameraStateData, kViewProjMatUnjittered, 0x140);
	static_assert_offset(CameraStateData, kProjMatUnjittered, 0x1C0);
	static_assert_offset(CameraStateData, PosAdjust, 0x260);
	static_assert_offset(CameraStateData, PreviousPosAdjust, 0x278);
	static_assert_offset(CameraStateData, UseJitter, 0x284);

	struct State
	{
		NiPointer<NiSourceTexture> pDefaultTextureProjNoiseMap;
		NiPointer<NiSourceTexture> pDefaultTextureProjDiffuseMap;
		NiPointer<NiSourceTexture> pDefaultTextureProjNormalMap;
		NiPointer<NiSourceTexture> pDefaultTextureProjNormalDetailMap;
		char _pad0[0x2C];
		uint32_t uiFrameCount;
		bool bInsideFrame;
		bool bLetterbox;
		bool bUnknown1;
		bool bCompiledShaderThisFrame;
		bool bUseEarlyZ;
		NiPointer<NiSourceTexture> pDefaultTextureBlack;// "BSShader_DefHeightMap"
		NiPointer<NiSourceTexture> pDefaultTextureWhite;
		NiPointer<NiSourceTexture> pDefaultTextureGrey;
		NiPointer<NiSourceTexture> pDefaultHeightMap;
		NiPointer<NiSourceTexture> pDefaultReflectionCubeMap;
		NiPointer<NiSourceTexture> pDefaultFaceDetailMap;
		NiPointer<NiSourceTexture> pDefaultTexEffectMap;
		NiPointer<NiSourceTexture> pDefaultTextureNormalMap;
		NiPointer<NiSourceTexture> pDefaultTextureDitherNoiseMap;
		BSTArray<CameraStateData> kCameraDataCacheA;
		char _pad2[0x4];// unknown dword
		float fHaltonSequence[2][8]; // (2, 3) Halton Sequence points
		float fDynamicResolutionWidth;
		float fDynamicResolutionHeight;
		float fDynamicResolutionPreviousWidth;
		float fDynamicResolutionPreviousHeight;
		uint32_t uiDynamicResolutionUnknown1;
		uint32_t uiDynamicResolutionUnknown2;
		uint16_t usDynamicResolutionUnknown3;

		CameraStateData *FindCameraDataCache(const NiCamera *Camera, bool UseJitter);
		void SetCameraData(const NiCamera *Camera, uint32_t StateFlags);
	};
	static_assert(sizeof(State) == 0x118);
	static_assert_offset(State, pDefaultTextureProjNoiseMap, 0x0);
	static_assert_offset(State, pDefaultTextureProjDiffuseMap, 0x8);
	static_assert_offset(State, pDefaultTextureProjNormalMap, 0x10);
	static_assert_offset(State, pDefaultTextureProjNormalDetailMap, 0x18);
	static_assert_offset(State, uiFrameCount, 0x4C);
	static_assert_offset(State, bInsideFrame, 0x50);
	static_assert_offset(State, bLetterbox, 0x51);
	static_assert_offset(State, bUnknown1, 0x52);
	static_assert_offset(State, bCompiledShaderThisFrame, 0x53);
	static_assert_offset(State, bUseEarlyZ, 0x54);
	static_assert_offset(State, pDefaultTextureBlack, 0x58);
	static_assert_offset(State, pDefaultTextureWhite, 0x60);
	static_assert_offset(State, pDefaultTextureGrey, 0x68);
	static_assert_offset(State, pDefaultHeightMap, 0x70);
	static_assert_offset(State, pDefaultReflectionCubeMap, 0x78);
	static_assert_offset(State, pDefaultFaceDetailMap, 0x80);
	static_assert_offset(State, pDefaultTexEffectMap, 0x88);
	static_assert_offset(State, pDefaultTextureNormalMap, 0x90);
	static_assert_offset(State, pDefaultTextureDitherNoiseMap, 0x98);
	static_assert_offset(State, kCameraDataCacheA, 0xA0);
	static_assert_offset(State, fHaltonSequence, 0xBC);
	static_assert_offset(State, fDynamicResolutionWidth, 0xFC);
	static_assert_offset(State, fDynamicResolutionHeight, 0x100);
	static_assert_offset(State, fDynamicResolutionPreviousWidth, 0x104);
	static_assert_offset(State, fDynamicResolutionPreviousHeight, 0x108);
	static_assert_offset(State, uiDynamicResolutionUnknown1, 0x10C);
	static_assert_offset(State, uiDynamicResolutionUnknown2, 0x110);
	static_assert_offset(State, usDynamicResolutionUnknown3, 0x114);

	AutoPtr(State, gState, 0x3052890);					// Used everywhere
	AutoPtr(CameraStateData, gCameraState, 0x30529B0);	// Only used in BSGraphics::State::SetCameraData
}