namespace BSBloodSplatterShader
{
	namespace VSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "WorldViewProj";
			case 1:return "LightLoc";
			case 2:return "Ctrl";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
				return 4;
			case 1:
			case 2:
				return 1;
			}

			return 0;
		}
	}

	namespace PSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "Alpha";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:return 1;
			}

			return 0;
		}
	}

	namespace Samplers
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "BloodColor";
			case 1:return "BloodAlpha";
			case 2:return "FlareColor";
			case 3:return "FlareHDR";
			}

			return BSSM_PLACEHOLDER;
		}
	}

	namespace Techniques
	{
		static void GetString(uint32_t Technique, char *Buffer, size_t BufferSize)
		{
			switch (Technique)
			{
			case 0:strcpy_s(Buffer, BufferSize, "Splatter"); break;
			case 1:strcpy_s(Buffer, BufferSize, "Flare"); break;
			default:__debugbreak(); break;
			}

			if (Technique)
				strcat_s(Buffer, BufferSize, " AlphaTest");
		}
	}

	namespace Defines
	{
		static auto GetArray(uint32_t Technique)
		{
			std::vector<std::pair<const char *, const char *>> defines;

			switch (Technique)
			{
			case 0:defines.emplace_back("SPLATTER", ""); break;
			case 1:defines.emplace_back("FLARE", ""); break;
			default:/* Apparently returns nothing */break;
			}

			return defines;
		}
	}
}

namespace BSDistantTreeShader
{
	namespace VSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "InstanceData";
			case 1:return "WorldViewProj";
			case 2:return "World";
			case 3:return "PreviousWorld";
			case 4:return "FogParam";
			case 5:return "FogNearColor";
			case 6:return "FogFarColor";
			case 7:return "DiffuseDir";
			case 8:return "IndexScale";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
				return 150;
			case 1:
			case 2:
			case 3:
				return 4;
			case 4:
			case 5:
			case 6:
			case 7:
				return 1;
			}

			return 0;
		}
	}

	namespace PSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "DiffuseColor";
			case 1:return "AmbientColor";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
			case 1:
				return 1;
			}

			return 0;
		}
	}

	namespace Samplers
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "Diffuse";
			}

			return BSSM_PLACEHOLDER;
		}
	}

	namespace Techniques
	{
		static void GetString(uint32_t Technique, char *Buffer, size_t BufferSize)
		{
			switch (Technique & ~DO_ALPHA_TEST_FLAG)
			{
			case 0:strcpy_s(Buffer, BufferSize, "DistantTreeBlock"); break;
			case 1:strcpy_s(Buffer, BufferSize, "Depth"); break;
			default:__debugbreak(); break;
			}

			if (Technique & DO_ALPHA_TEST_FLAG)
				strcat_s(Buffer, BufferSize, " AlphaTest");
		}
	}

	namespace Defines
	{
		static auto GetArray(uint32_t Technique)
		{
			std::vector<std::pair<const char *, const char *>> defines;

			switch (Technique & ~DO_ALPHA_TEST_FLAG)
			{
			case 0:break;
			case 1:defines.emplace_back("RENDER_DEPTH", ""); break;
			default:__debugbreak(); break;
			}

			if (Technique & DO_ALPHA_TEST_FLAG)
				defines.emplace_back("DO_ALPHA_TEST", "");

			return defines;
		}
	}
}

namespace BSGrassShader
{
	namespace VSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "WorldViewProj";
			case 1:return "WorldView";
			case 2:return "World";
			case 3:return "PreviousWorld";
			case 4:return "FogNearColor";
			case 5:return "WindVector";
			case 6:return "WindTimer";
			case 7:return "DirLightDirection";
			case 8:return "PreviousWindTimer";
			case 9:return "DirLightColor";
			case 10:return "AlphaParam1";
			case 11:return "AmbientColor";
			case 12:return "AlphaParam2";
			case 13:return "ScaleMask";
			case 14:return "padding";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				return 4;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
				return 1;
			}

			return 0;
		}
	}

	namespace PSConstants
	{
		static const char *GetString(int Index)
		{
			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			return 0;
		}
	}

	namespace Samplers
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "BaseSampler";
			case 1:return "ShadowMaskSampler";
			}

			return BSSM_PLACEHOLDER;
		}
	}

	namespace Techniques
	{
		static void GetString(uint32_t Technique, char *Buffer, size_t BufferSize)
		{
			switch (Technique & ~DO_ALPHA_TEST_FLAG)
			{
			case 0:strcpy_s(Buffer, BufferSize, "VertexL"); break;
			case 1:strcpy_s(Buffer, BufferSize, "FlatL"); break;
			case 2:strcpy_s(Buffer, BufferSize, "FlatL_Slope"); break;
			case 3:strcpy_s(Buffer, BufferSize, "VertexL_Slope"); break;
			case 4:strcpy_s(Buffer, BufferSize, "VertexL_Billboard"); break;
			case 5:strcpy_s(Buffer, BufferSize, "FlatL_Billboard"); break;
			case 6:strcpy_s(Buffer, BufferSize, "FlatL_Slope_Billboard"); break;
			case 7:strcpy_s(Buffer, BufferSize, "VertexL_Slope_Billboard"); break;
			case 8:strcpy_s(Buffer, BufferSize, "RenderDepth"); break;
			default:__debugbreak(); break;
			}

			if (Technique & DO_ALPHA_TEST_FLAG)
				strcat_s(Buffer, BufferSize, " AlphaTest");
		}
	}

	namespace Defines
	{
		static auto GetArray(uint32_t Technique)
		{
			std::vector<std::pair<const char *, const char *>> defines;

			switch (Technique & ~DO_ALPHA_TEST_FLAG)
			{
			case 0:defines.emplace_back("VERTLIT", ""); break;
			case 1:break;
			case 2:defines.emplace_back("SOPE", ""); break;
			case 3:defines.emplace_back("VERTLIT", ""); defines.emplace_back("SLOPE", ""); break;
			case 4:defines.emplace_back("VERTLIT", ""); defines.emplace_back("SLOPE", ""); defines.emplace_back("BILLBOARD", ""); break;
			case 5:defines.emplace_back("BILLBOARD", ""); break;
			case 6:defines.emplace_back("SLOPE", ""); defines.emplace_back("BILLBOARD", ""); break;
			case 7:defines.emplace_back("VERTLIT", ""); defines.emplace_back("SLOPE", ""); defines.emplace_back("BILLBOARD", ""); break;
			case 8:defines.emplace_back("RENDER_DEPTH", ""); break;
			default:__debugbreak(); break;
			}

			if (Technique & DO_ALPHA_TEST_FLAG)
				defines.emplace_back("DO_ALPHA_TEST", "");

			return defines;
		}
	}
}

namespace BSParticleShader
{
	namespace VSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "WorldViewProj";
			case 1:return "PrevWorldViewProj";
			case 2:return "PrecipitationOcclusionWorldViewProj";
			case 3:return "fVars0";
			case 4:return "fVars1";
			case 5:return "fVars2";
			case 6:return "fVars3";
			case 7:return "fVars4";
			case 8:return "Color1";
			case 9:return "Color2";
			case 10:return "Color3";
			case 11:return "Velocity";
			case 12:return "Acceleration";
			case 13:return "ScaleAdjust";
			case 14:return "Wind";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
			case 1:
			case 2:
				return 4;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
				return 1;
			}

			return 0;
		}
	}

	namespace PSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "ColorScale";
			case 1:return "TextureSize";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			return !Index || Index == 1;
		}
	}

	namespace Samplers
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "SourceTexture";
			case 1:return "GrayscaleTexture";
			case 2:return "PrecipitationOcclusionTexture";
			case 3:return "UnderwaterMask";
			}

			return BSSM_PLACEHOLDER;
		}
	}

	namespace Techniques
	{
		static void GetString(uint32_t Technique, char *Buffer, size_t BufferSize)
		{
			switch (Technique)
			{
			case 0:strcpy_s(Buffer, BufferSize, "Particles"); break;
			case 1:strcpy_s(Buffer, BufferSize, "ParticlesGryColor"); break;
			case 2:strcpy_s(Buffer, BufferSize, "ParticlesGryAlpha"); break;
			case 3:strcpy_s(Buffer, BufferSize, "ParticlesGryColorAlpha"); break;
			case 4:strcpy_s(Buffer, BufferSize, "EnvCubeSnow"); break;
			case 5:strcpy_s(Buffer, BufferSize, "EnvCubeRain"); break;
			default:__debugbreak(); break;
			}
		}
	}

	namespace Defines
	{
		static auto GetArray(uint32_t Technique)
		{
			std::vector<std::pair<const char *, const char *>> defines;

			switch (Technique)
			{
			case 0:break;
			case 1:defines.emplace_back("GRAYSCALE_TO_COLOR", ""); break;
			case 2:defines.emplace_back("GRAYSCALE_TO_ALPHA", ""); break;
			case 3:defines.emplace_back("GRAYSCALE_TO_COLOR", ""); defines.emplace_back("GRAYSCALE_TO_ALPHA", ""); break;
			case 4:defines.emplace_back("ENVCUBE", ""); defines.emplace_back("SNOW", ""); break;
			case 5:defines.emplace_back("ENVCUBE", ""); defines.emplace_back("RAIN", ""); break;
			default:__debugbreak(); break;
			}

			return defines;
		}
	}
}

namespace BSSkyShader
{
	namespace VSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "WorldViewProj";
			case 1:return "World";
			case 2:return "PreviousWorld";
			case 3:return "BlendColor";
			case 4:return "EyePosition";
			case 5:return "TexCoordOff";
			case 6:return "VParams";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
			case 1:
			case 2:
				return 4;
			case 3:
				return 3;
			case 4:
			case 5:
			case 6:
				return 1;
			}

			return 0;
		}
	}

	namespace PSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "PParams";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			return Index == 0;
		}
	}

	namespace Samplers
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "BaseSampler";
			case 1:return "BlendSampler";
			case 2:return "NoiseGradSampler";
			}

			return BSSM_PLACEHOLDER;
		}
	}

	namespace Techniques
	{
		static void GetString(uint32_t Technique, char *Buffer, size_t BufferSize)
		{
			switch (Technique)
			{
			case 0:strcpy_s(Buffer, BufferSize, "SunOcclude"); break;
			case 1:strcpy_s(Buffer, BufferSize, "SunGlare"); break;
			case 2:strcpy_s(Buffer, BufferSize, "MoonAndStarsMask"); break;
			case 3:strcpy_s(Buffer, BufferSize, "Stars"); break;
			case 4:strcpy_s(Buffer, BufferSize, "Clouds"); break;
			case 5:strcpy_s(Buffer, BufferSize, "CloudsLerp"); break;
			case 6:strcpy_s(Buffer, BufferSize, "CloudsFade"); break;
			case 7:strcpy_s(Buffer, BufferSize, "Texture"); break;
			case 8:strcpy_s(Buffer, BufferSize, "Sky"); break;
			default:__debugbreak(); break;
			}
		}
	}

	namespace Defines
	{
		static auto GetArray(uint32_t Technique)
		{
			std::vector<std::pair<const char *, const char *>> defines;

			switch (Technique)
			{
			case 0:defines.emplace_back("OCCLUSION", ""); break;
			case 1:defines.emplace_back("TEX", ""); defines.emplace_back("DITHER", ""); break;
			case 2:defines.emplace_back("TEX", ""); defines.emplace_back("MOONMASK", ""); break;
			case 3:defines.emplace_back("HORIZFADE", ""); break;
			case 4:defines.emplace_back("TEX", ""); defines.emplace_back("CLOUDS", ""); break;
			case 5:defines.emplace_back("TEX", ""); defines.emplace_back("CLOUDS", ""); defines.emplace_back("TEXLERP", ""); break;
			case 6:defines.emplace_back("TEX", ""); defines.emplace_back("CLOUDS", ""); defines.emplace_back("TEXFADE", ""); break;
			case 7:defines.emplace_back("TEX", ""); break;
			case 8:defines.emplace_back("DITHER", ""); break;
			default:__debugbreak(); break;
			}

			return defines;
		}
	}
}

namespace BSXShader
{
	namespace VSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "World";
			case 1:return "PreviousWorld";
			case 2:return "Bones";
			case 3:return "EyePosition";
			case 4:return "FogParam";
			case 5:return "FogNearColor";
			case 6:return "FogFarColor";
			case 7:return "FalloffData";
			case 8:return "SoftMaterialVSParams";
			case 9:return "TexcoordOffset";
			case 10:return "TexcoordOffsetMembrane";
			case 11:return "SubTexOffset";
			case 12:return "PosAdjust";
			case 13:return "MatProj";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
			case 1:
			case 13:
				return 3;
			case 3:
			case 12:
				return 1;
			}

			return 0;
		}
	}

	namespace PSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "PropertyColor";
			case 1:return "AlphaTestRef";
			case 2:return "MembraneRimColor";
			case 3:return "MembraneVars";
			case 4:return "PLightPositionX";
			case 5:return "PLightPositionY";
			case 6:return "PLightPositionZ";
			case 7:return "PLightingRadiusInverseSquared";
			case 8:return "PLightColorR";
			case 9:return "PLightColorG";
			case 10:return "PLightColorB";
			case 11:return "DLightColor";
			case 12:return "VPOSOffset";
			case 13:return "CameraData";
			case 14:return "FilteringParam";
			case 15:return "BaseColor";
			case 16:return "BaseColorScale";
			case 17:return "LightingInfluence";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
				return 1;
			}

			return 0;
		}
	}

	namespace Samplers
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "BaseSampler";
			case 1:return "NormalSampler";
			case 2:return "NoiseSampler";
			case 3:return "DepthSampler";
			case 4:return "GrayscaleSampler";
			}

			return BSSM_PLACEHOLDER;
		}
	}

	namespace Techniques
	{
		static void GetString(uint32_t Technique, char *Buffer, size_t BufferSize)
		{
			LONG bits = Technique;

			strcpy_s(Buffer, BufferSize, "");

			if (Technique & 0x1)
				strcat_s(Buffer, BufferSize, "Vc ");
			if (Technique & 0x80)
				strcat_s(Buffer, BufferSize, "I ");
			if (Technique & 0x40)
				strcat_s(Buffer, BufferSize, "Tex ");
			if (Technique & 0x8)
				strcat_s(Buffer, BufferSize, "Sk ");
			if (Technique & 0x10)
				strcat_s(Buffer, BufferSize, "N ");
			if (Technique & 0x20)
				strcat_s(Buffer, BufferSize, "BT ");
			if (_bittest(&bits, 8))
				strcat_s(Buffer, BufferSize, "Fo ");
			if (_bittest(&bits, 10))
				strcat_s(Buffer, BufferSize, "Ab ");
			if (_bittest(&bits, 11))
				strcat_s(Buffer, BufferSize, "Mb ");
			if (_bittest(&bits, 12))
				strcat_s(Buffer, BufferSize, "Ptcl ");
			if (_bittest(&bits, 13))
				strcat_s(Buffer, BufferSize, "Sptcl ");
			if (_bittest(&bits, 14))
				strcat_s(Buffer, BufferSize, "Blood ");
			if (_bittest(&bits, 15))
				strcat_s(Buffer, BufferSize, "Mem ");
			if (_bittest(&bits, 16))
				strcat_s(Buffer, BufferSize, "Lit ");
			if (_bittest(&bits, 17))
				strcat_s(Buffer, BufferSize, "Projuv ");
			if (_bittest(&bits, 18))
				strcat_s(Buffer, BufferSize, "Soft ");

			if ((Technique & 0x180000) == 0x180000)
				strcat_s(Buffer, BufferSize, "Grayca ");
			else if (_bittest(&bits, 19))
				strcat_s(Buffer, BufferSize, "Grayc ");
			else if (_bittest(&bits, 20))
				strcat_s(Buffer, BufferSize, "Graya ");

			if (_bittest(&bits, 21))
				strcat_s(Buffer, BufferSize, "Notexa ");
			if (_bittest(&bits, 22))
				strcat_s(Buffer, BufferSize, "Mbdecal ");
			if (_bittest(&bits, 23))
				strcat_s(Buffer, BufferSize, "At ");
			if (_bittest(&bits, 24))
				strcat_s(Buffer, BufferSize, "Sky ");
			if (_bittest(&bits, 26))
				strcat_s(Buffer, BufferSize, "Opaque ");

			Trim(Buffer, ' ');
		}
	}

	namespace Defines
	{
		static auto GetArray(uint32_t Technique)
		{
			std::vector<std::pair<const char *, const char *>> defines;

			if (TEST_BIT(0)) defines.emplace_back("VC", "");
			if (TEST_BIT(1)) defines.emplace_back("TEXCOORD", "");
			if (TEST_BIT(2)) defines.emplace_back("TEXCOORD_INDEX", "");
			if (TEST_BIT(3)) defines.emplace_back("SKINNED", "");
			if (TEST_BIT(4)) defines.emplace_back("NORMALS", "");
			if (TEST_BIT(5)) defines.emplace_back("BINORMAL_TANGENT", "");
			if (TEST_BIT(6)) defines.emplace_back("TEXTURE", "");
			if (TEST_BIT(7)) defines.emplace_back("INDEXED_TEXTURE", "");
			if (TEST_BIT(8)) defines.emplace_back("FALLOFF", "");
			// 9?
			if (TEST_BIT(10)) defines.emplace_back("ADDBLEND", "");
			if (TEST_BIT(11)) defines.emplace_back("MULTBLEND", "");
			if (TEST_BIT(12)) defines.emplace_back("PARTICLES", "");
			if (TEST_BIT(13)) defines.emplace_back("STRIP_PARTICLES", "");
			if (TEST_BIT(14)) defines.emplace_back("BLOOD", "");
			if (TEST_BIT(15)) defines.emplace_back("MEMBRANE", "");
			if (TEST_BIT(16)) defines.emplace_back("LIGHTING", "");
			if (TEST_BIT(17)) defines.emplace_back("PROJECTED_UV", "");
			if (TEST_BIT(18)) defines.emplace_back("SOFT", "");
			if (TEST_BIT(19)) defines.emplace_back("GRAYSCALE_TO_COLOR", "");
			if (TEST_BIT(20)) defines.emplace_back("GRAYSCALE_TO_ALPHA", "");
			if (TEST_BIT(21)) defines.emplace_back("IGNORE_TEX_ALPHA", "");
			if (TEST_BIT(22)) defines.emplace_back("MULTBLEND_DECAL", "");
			if (TEST_BIT(23)) defines.emplace_back("ALPHA_TEST", "");
			if (TEST_BIT(24)) defines.emplace_back("SKY_OBJECT", "");
			if (TEST_BIT(25)) defines.emplace_back("MSN_SPU_SKINNED", "");
			if (TEST_BIT(26)) defines.emplace_back("MOTIONVECTORS_NORMALS", "");

			return defines;
		}
	}
}

namespace BSLightingShader
{
	namespace VSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "WorldViewProj";
			case 1:return "PrevWorldViewProj";
			case 2:return "PrecipitationOcclusionWorldViewProj";
			case 3:return "fVars0";
			case 4:return "fVars1";
			case 5:return "fVars2";
			case 6:return "fVars3";
			case 7:return "fVars4";
			case 8:return "Color1";
			case 9:return "Color2";
			case 10:return "Color3";
			case 11:return "Velocity";
			case 12:return "Acceleration";
			case 13:return "ScaleAdjust";
			case 14:return "Wind";
			case 15:return "UNKNOWN_NAME";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
			case 1:
			case 6:
				return 3;
			case 2:
			case 3:
			case 4:
			case 5:
				return 1;
			}

			return 0;
		}
	}

	namespace PSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "NumLightNumShadowLight";
			case 1:return "PointLightPosition";
			case 2:return "PointLightColor";
			case 3:return "DirLightDirection";
			case 4:return "DirLightColor";
			case 5:return "DirectionalAmbient";
			case 6:return "AmbientSpecularTintAndFresnelPower";
			case 7:return "MaterialData";
			case 8:return "EmitColor";
			case 9:return "AlphaTestRef";
			case 10:return "ShadowLightMaskSelect";
			case 11:return "VPOSOffset";
			case 12:return "ProjectedUVParams";
			case 13:return "ProjectedUVParams2";
			case 14:return "ProjectedUVParams3";
			case 15:return "SplitDistance";
			case 16:return "SSRParams";
			case 17:return "WorldMapOverlayParametersPS";
			case 18:return "AmbientColor";
			case 19:return "FogColor";
			case 20:return "ColourOutputClamp";
			case 21:return "EnvmapData";
			case 22:return "ParallaxOccData";
			case 23:return "TintColor";
			case 24:return "LODTexParams";
			case 25:return "SpecularColor";
			case 26:return "SparkleParams";
			case 27:return "MultiLayerParallaxData";
			case 28:return "LightingEffectParams";
			case 29:return "IBLParams";
			case 30:return "LandscapeTexture1to4IsSnow";
			case 31:return "LandscapeTexture5to6IsSnow";
			case 32:return "LandscapeTexture1to4IsSpecPower";
			case 33:return "LandscapeTexture5to6IsSpecPower";
			case 34:return "SnowRimLightParameters";
			case 35:return "CharacterLightParams";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, uint32_t Technique)
		{
			int v2 = 1;

			// PointLightPosition/PointLightColor variable size array
			if ((unsigned int)(Index - 1) <= 1)
				v2 = (Technique >> 3) & 7;

			switch (Index)
			{
			case 5:
				return 3;
			case 6:
			case 16:
			case 17:
			case 18:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
			case 24:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
				return 0;
			}

			return v2;
		}
	}

	namespace Samplers
	{
		static const char *GetString(int Index, uint32_t Technique)
		{
			uint32_t subType = (Technique >> 24) & 0x3F;

			if (TEST_BIT(18))
			{
				switch (Index)
				{
				case 12:return "WorldMapOverlayNormalSampler";
				case 13:return "WorldMapOverlayNormalSnowSampler";
				}
			}

			if (TEST_BIT(15) && subType != 6)
			{
				switch (Index)
				{
				case 3:return "ProjectedDiffuseSampler";
				case 8:return "ProjectedNormalSampler";
				case 10:return "ProjectedNormalDetailSampler";
				case 11:return "ProjectedNoiseSampler";
				}
			}

			switch (Index)
			{
			case 0:return "DiffuseSampler";
			case 1:return "NormalSampler";
			case 2:return "SpecularSampler";
			case 3:return "HeightSampler";
			case 4:return "EnvSampler";
			case 5:return "EnvMaskSampler";
			case 6:return "GlowSampler";
			case 7:return "LandscapeNormalSampler";
			case 8:return "MultiLayerParallaxSampler";
			case 9:return "BackLightMaskSampler";
			case 10:return "ProjectedNormalDetailSampler";
			case 11:return "ProjectedNoiseSampler";
			case 12:return "SubSurfaceSampler";
			case 13:return "LODBlendSampler";
			case 14:return "ShadowMaskSampler";
			case 15:return "LODNoiseSampler";
			}

			return BSSM_PLACEHOLDER;
		}
	}

	namespace Techniques
	{
		static void GetString(uint32_t Technique, char *Buffer, size_t BufferSize)
		{
			sprintf_s(Buffer, BufferSize, "%dSh%d ", (Technique >> 3) & 7, (Technique >> 6) & 7);
			uint32_t subType = (Technique >> 24) & 0x3F;

			if (TEST_BIT(0))
				strcat_s(Buffer, BufferSize, "Vc ");
			if (TEST_BIT(1))
				strcat_s(Buffer, BufferSize, "Sk ");
			if (TEST_BIT(2))
				strcat_s(Buffer, BufferSize, "Msn ");
			if (TEST_BIT(9))
				strcat_s(Buffer, BufferSize, "Spc ");
			if (TEST_BIT(10))
				strcat_s(Buffer, BufferSize, "Sss ");
			if (TEST_BIT(13))
				strcat_s(Buffer, BufferSize, "Shd ");
			if (TEST_BIT(14))
				strcat_s(Buffer, BufferSize, "DfSh ");
			if (TEST_BIT(11))
				strcat_s(Buffer, BufferSize, "Rim ");
			if (TEST_BIT(12))
				strcat_s(Buffer, BufferSize, "Bk ");
			if (TEST_BIT(15) && subType != 6)
				strcat_s(Buffer, BufferSize, "Projuv ");
			if (TEST_BIT(17))
				strcat_s(Buffer, BufferSize, "Aspc ");
			if (TEST_BIT(18))
				strcat_s(Buffer, BufferSize, "Wmap ");
			if (TEST_BIT(20))
				strcat_s(Buffer, BufferSize, "Atest ");
			if (TEST_BIT(21))
				strcat_s(Buffer, BufferSize, "Snow ");
			if (TEST_BIT(19))
				strcat_s(Buffer, BufferSize, "BaseSnow ");
			if (TEST_BIT(15) && subType == 6)
				strcat_s(Buffer, BufferSize, "DwDecals ");
			if (TEST_BIT(23))
				strcat_s(Buffer, BufferSize, "Aam ");

			switch (subType)
			{
			case 0:strcat_s(Buffer, BufferSize, "None "); break;
			case 1:strcat_s(Buffer, BufferSize, "Envmap "); break;
			case 2:strcat_s(Buffer, BufferSize, "Glowmap "); break;
			case 3:strcat_s(Buffer, BufferSize, "Parallax "); break;
			case 4:strcat_s(Buffer, BufferSize, "Facegen "); break;
			case 5:strcat_s(Buffer, BufferSize, "FacegenRGBTint "); break;
			case 6:strcat_s(Buffer, BufferSize, "Hair "); break;
			case 7:strcat_s(Buffer, BufferSize, "ParallaxOcc "); break;
			case 8:strcat_s(Buffer, BufferSize, "MTLand "); break;
			case 9:strcat_s(Buffer, BufferSize, "LODLand "); break;
				// 10 is missing. Wtf?
			case 11:strcat_s(Buffer, BufferSize, "MultiLayerParallax "); break;
			case 12:strcat_s(Buffer, BufferSize, "Tree "); break;
			case 13:strcat_s(Buffer, BufferSize, "LODObj "); break;
			case 14:strcat_s(Buffer, BufferSize, "MultiIndexTriShapeSnow "); break;
			case 15:strcat_s(Buffer, BufferSize, "LODObjHD "); break;
			case 16:strcat_s(Buffer, BufferSize, "Eye "); break;
				// 17 is missing. Wtf?
			case 18:strcat_s(Buffer, BufferSize, "LODLandNoise "); break;
			case 19:strcat_s(Buffer, BufferSize, "MTLandLODBlend "); break;
			default:strcat_s(Buffer, BufferSize, "? "); break;
			}

			Trim(Buffer, ' ');
		}
	}

	namespace Defines
	{
		static auto GetArray(uint32_t Technique)
		{
			std::vector<std::pair<const char *, const char *>> defines;
			uint32_t subType = (Technique >> 24) & 0x3F;

			if (TEST_BIT(0)) defines.emplace_back("VC", "");
			if (TEST_BIT(1)) defines.emplace_back("SKINNED", "");
			if (TEST_BIT(2)) defines.emplace_back("MODELSPACENORMALS", "");
			if (TEST_BIT(9)) defines.emplace_back("SPECULAR", "");
			if (TEST_BIT(10)) defines.emplace_back("SOFT_LIGHTING", "");
			if (TEST_BIT(13)) defines.emplace_back("SHADOW_DIR", "");
			if (TEST_BIT(14)) defines.emplace_back("DEFSHADOW", "");
			if (TEST_BIT(11)) defines.emplace_back("RIM_LIGHTING", "");
			if (TEST_BIT(12)) defines.emplace_back("BACK_LIGHTING", "");
			if (TEST_BIT(15) && subType != 6) defines.emplace_back("PROJECTED_UV", "");
			if (TEST_BIT(16)) defines.emplace_back("ANISO_LIGHTING", "");
			if (TEST_BIT(17)) defines.emplace_back("AMBIENT_SPECULAR", "");
			if (TEST_BIT(18)) defines.emplace_back("WORLD_MAP", "");
			if (TEST_BIT(20)) defines.emplace_back("DO_ALPHA_TEST", "");
			if (TEST_BIT(21)) defines.emplace_back("SNOW", "");
			if (TEST_BIT(19)) defines.emplace_back("BASE_OBJECT_IS_SNOW", "");
			if (TEST_BIT(22)) defines.emplace_back("CHARACTER_LIGHT", "");
			if (TEST_BIT(15) && subType == 6) defines.emplace_back("DEPTH_WRITE_DECALS", "");
			if (TEST_BIT(23)) defines.emplace_back("ADDITIONAL_ALPHA_MASK", "");

			switch (subType)
			{
			case 0:break;
			case 1:defines.emplace_back("ENVMAP", ""); break;
			case 2:defines.emplace_back("GLOWMAP", ""); break;
			case 3:defines.emplace_back("PARALLAX", ""); break;
			case 4:defines.emplace_back("FACEGEN", ""); break;
			case 5:defines.emplace_back("FACEGEN_RGB_TINT", ""); break;
			case 6:defines.emplace_back("HAIR", ""); break;
			case 7:defines.emplace_back("PARALLAX_OCC", ""); break;
			case 8:defines.emplace_back("MULTI_TEXTURE", ""); defines.emplace_back("LANDSCAPE", ""); break;
			case 9:defines.emplace_back("LODLANDSCAPE", ""); break;
			case 10:/* I have no idea what this does */break;
			case 11:defines.emplace_back("MULTI_LAYER_PARALLAX", ""); defines.emplace_back("ENVMAP", ""); break;
			case 12:defines.emplace_back("TREE_ANIM", ""); break;
			case 13:defines.emplace_back("LODOBJECTS", ""); break;
			case 14:defines.emplace_back("MULTI_INDEX", "SPARKLE"); defines.emplace_back("SPARKLE", ""); break;
			case 15:defines.emplace_back("LODOBJECTSHD", ""); break;
			case 16:defines.emplace_back("EYE", ""); break;
			case 17:defines.emplace_back("CLOUD", ""); defines.emplace_back("INSTANCED", ""); break;
			case 18:defines.emplace_back("LODLANDSCAPE", ""); defines.emplace_back("LODLANDNOISE", ""); break;
			case 19:defines.emplace_back("MULTI_TEXTURE", ""); defines.emplace_back("LANDSCAPE", "LOD_LAND_BLEND"); break;
			default:__debugbreak(); break;
			}

			return defines;
		}
	}
}

namespace BSUtilityShader
{
	namespace VSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "World";
			case 1:return "TexcoordOffset";
			case 2:return "EyePos";
			case 3:return "HighDetailRange";
			case 4:return "ParabolaParam";
			case 5:return "ShadowFadeParam";
			case 6:return "TreeParams";
			case 7:return "WaterParams";
			case 8:return "Bones";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
				return 3;
			case 2:
			case 5:
			case 6:
			case 7:
				return 1;
			}

			return 0;
		}
	}

	namespace PSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "AlphaTestRef";
			case 1:return "RefractionPower";
			case 2:return "DebugColor";
			case 3:return "BaseColor";
			case 4:return "PropertyColor";
			case 5:return "FocusShadowMapProj";
			case 6:return "ShadowMapProj";
			case 7:return "ShadowSampleParam";
			case 8:return "ShadowLightParam";
			case 9:return "ShadowFadeParam";
			case 10:return "VPOSOffset";
			case 11:return "EndSplitDistances";
			case 12:return "StartSplitDistances";
			case 13:return "FocusShadowFadeParam";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			if (Index == 6 && Flags & (1u << 21))
				return 9;

			switch (Index)
			{
			case 0:
			case 2:
			case 3:
			case 4:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
				return 1;
			case 5:
				return 12;
			case 6:
				return 4;
			}

			return 0;
		}
	}

	namespace Samplers
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "BaseSampler";
			case 1:return "NormalSampler";
			case 2:return "DepthSampler";
			case 3:return "ShadowMapSampler";
			case 4:return "ShadowMapSamplerComp";
			case 5:return "StencilSampler";
			case 6:return "FocusShadowMapSamplerComp";
			case 7:return "GrayscaleSampler";
			}

			return BSSM_PLACEHOLDER;
		}
	}

	namespace Techniques
	{
		static void GetString(uint32_t Technique, char *Buffer, size_t BufferSize)
		{
			LONG bits = Technique;

			strcpy_s(Buffer, BufferSize, "");

			if (Technique & 0x1)
				strcat_s(Buffer, BufferSize, "Vc ");
			if (Technique & 0x2)
				strcat_s(Buffer, BufferSize, "T ");
			if (Technique & 0x4)
				strcat_s(Buffer, BufferSize, "Sk ");
			if (Technique & 0x8)
				strcat_s(Buffer, BufferSize, "N ");
			if (Technique & 0x10)
				strcat_s(Buffer, BufferSize, "BT ");
			if (Technique & 0x20)
				strcat_s(Buffer, BufferSize, "L ");
			if (Technique & 0x80)
				strcat_s(Buffer, BufferSize, "At ");
			if (_bittest(&bits, 8))
				strcat_s(Buffer, BufferSize, "Llod ");
			if (Technique & 0x200)
				strcat_s(Buffer, BufferSize, "Norm ");
			if (_bittest(&bits, 10))
				strcat_s(Buffer, BufferSize, "NFall ");
			if (_bittest(&bits, 11))
				strcat_s(Buffer, BufferSize, "NClamp ");
			if (Technique & 0x1000)
				strcat_s(Buffer, BufferSize, "NClear ");
			if (_bittest(&bits, 13))
				strcat_s(Buffer, BufferSize, "Depth ");
			if (_bittest(&bits, 14))
				strcat_s(Buffer, BufferSize, "Sm ");
			if (Technique & 0x8000)
				strcat_s(Buffer, BufferSize, "Smclamp ");
			if (_bittest(&bits, 16))
				strcat_s(Buffer, BufferSize, "Smpb ");
			if (_bittest(&bits, 17))
				strcat_s(Buffer, BufferSize, "DbgCol ");
			if (_bittest(&bits, 18))
				strcat_s(Buffer, BufferSize, "Pssm ");
			if (_bittest(&bits, 19))
				strcat_s(Buffer, BufferSize, "SilCol ");
			if (_bittest(&bits, 20))
				strcat_s(Buffer, BufferSize, "GryMsk ");
			if (_bittest(&bits, 21))
				strcat_s(Buffer, BufferSize, "Shdwmsk ");
			if (_bittest(&bits, 22))
				strcat_s(Buffer, BufferSize, "ShdwmskSpot ");
			if (_bittest(&bits, 23))
				strcat_s(Buffer, BufferSize, "ShdwmskPb ");
			if (_bittest(&bits, 24))
				strcat_s(Buffer, BufferSize, "ShdwmskDpb ");
			if (_bittest(&bits, 25))
				strcat_s(Buffer, BufferSize, "BaseTex ");
			if (_bittest(&bits, 28))
				strcat_s(Buffer, BufferSize, "FogOfWar ");
			if ((Technique & 0x1200) == 0x1200)
				strcat_s(Buffer, BufferSize, "Stencil ");
			if (_bittest(&bits, 29u))
				strcat_s(Buffer, BufferSize, "OpaqEffs ");
			if (Technique & 0x8000)
				strcat_s(Buffer, BufferSize, "OpqFGAlpha ");
			if ((Technique & 0x14000) == 0x10000)
				strcat_s(Buffer, BufferSize, "Aam ");

			Trim(Buffer, ' ');
		}
	}

	namespace Defines
	{
		static auto GetArray(uint32_t Technique)
		{
			std::vector<std::pair<const char *, const char *>> defines;

			if (TEST_BIT(0)) defines.emplace_back("VC", "");
			if (TEST_BIT(1)) defines.emplace_back("TEXTURE", "");
			if (TEST_BIT(2)) defines.emplace_back("SKINNED", "");
			if (TEST_BIT(3)) defines.emplace_back("NORMALS", "");
			if (TEST_BIT(4)) defines.emplace_back("BINORMAL_TANGENT", "");
			// 5,6 ?
			if (TEST_BIT(7)) defines.emplace_back("ALPHA_TEST", "");
			if (TEST_BIT(8) && (Technique & 0x600000)) defines.emplace_back("FOCUS_SHADOW", "");
			if (TEST_BIT(8)) defines.emplace_back("LOD_LANDSCAPE", "");
			if (TEST_BIT(9) && !TEST_BIT(12)) defines.emplace_back("RENDER_NORMAL", "");
			if (TEST_BIT(10)) defines.emplace_back("RENDER_NORMAL_FALLOFF", "");
			if (TEST_BIT(11)) defines.emplace_back("RENDER_NORMAL_CLAMP", "");
			if (TEST_BIT(12) && !TEST_BIT(9)) defines.emplace_back("RENDER_NORMAL_CLEAR", "");
			if (TEST_BIT(13)) defines.emplace_back("RENDER_DEPTH", "");

			if ((Technique & 0x20004000) == 0x4000)
			{
				defines.emplace_back("RENDER_SHADOWMAP", "");

				if (TEST_BIT(16))
					defines.emplace_back("RENDER_SHADOWMAP_PB", "");
			}
			else
			{
				if (TEST_BIT(16))
					defines.emplace_back("ADDITIONAL_ALPHA_MASK", "");
			}

			if (TEST_BIT(20)) defines.emplace_back("GRAYSCALE_MASK", "");
			if (TEST_BIT(21)) defines.emplace_back("RENDER_SHADOWMASK", "");
			if (TEST_BIT(22)) defines.emplace_back("RENDER_SHADOWMASKSPOT", "");
			if (TEST_BIT(23)) defines.emplace_back("RENDER_SHADOWMASKPB", "");
			if (TEST_BIT(24)) defines.emplace_back("RENDER_SHADOWMASKDPB", "");
			if (TEST_BIT(25)) defines.emplace_back("RENDER_BASE_TEXTURE", "");
			if (TEST_BIT(26)) defines.emplace_back("TREE_ANIM", "");
			if (TEST_BIT(27)) defines.emplace_back("LOD_OBJECT", "");
			if (TEST_BIT(28)) defines.emplace_back("LOCALMAP_FOGOFWAR", "");
			if (TEST_BIT(9) && TEST_BIT(12)) defines.emplace_back("STENCIL_ABOVE_WATER", "");

			if (TEST_BIT(29))
			{
				defines.emplace_back("OPAQUE_EFFECT", "");

				if (TEST_BIT(15))
					defines.emplace_back("GRAYSCALE_TO_ALPHA", "");
			}
			else
			{
				if (TEST_BIT(15))
					defines.emplace_back("RENDER_SHADOWMAP_CLAMPED", "");
			}

			if (Technique & 0x1E00000)
			{
				uint32_t temp = 0;
				if (TEST_BIT(17)) temp |= 1;
				if (TEST_BIT(18)) temp |= 2;
				if (TEST_BIT(19)) temp |= 4;

				switch ((temp <= 4) ? temp : 4)
				{
				case 0:defines.emplace_back("SHADOWFILTER", "0"); break;
				case 1:defines.emplace_back("SHADOWFILTER", "1"); break;
				case 2:defines.emplace_back("SHADOWFILTER", "2"); break;
				case 3:defines.emplace_back("SHADOWFILTER", "3"); break;
				case 4:defines.emplace_back("SHADOWFILTER", "4"); break;
				}
			}
			else if ((Technique & 0x20004000) == 0x4000 || TEST_BIT(13))
			{
				if (TEST_BIT(17))
					defines.emplace_back("DEPTH_WRITE_DECALS", "");
			}
			else
			{
				if (TEST_BIT(17) || TEST_BIT(19))
					defines.emplace_back("DEBUG_COLOR", "");

				if (TEST_BIT(18))
					defines.emplace_back("DEBUG_SHADOWSPLIT", "");
			}

			defines.emplace_back("SHADOWSPLITCOUNT", "3");

			if ((Technique & 0x14000) != 0x14000 &&
				((Technique & 0x20004000) == 0x4000 || (Technique & 0x1E02000) == 0x2000) &&
				!(Technique & 0x80) &&
				(Technique & 0x14000) != 0x10000)
				defines.emplace_back("NO_PIXEL_SHADER", "");

			return defines;
		}
	}
}

namespace BSWaterShader
{
	namespace VSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "WorldViewProj";
			case 1:return "World";
			case 2:return "PreviousWorld";
			case 3:return "QPosAdjust";
			case 4:return "ObjectUV";
			case 5:return "NormalsScroll0";
			case 6:return "NormalsScroll1";
			case 7:return "NormalsScale";
			case 8:return "VSFogParam";
			case 9:return "VSFogNearColor";
			case 10:return "VSFogFarColor";
			case 11:return "CellTexCoordOffset";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
			case 1:
			case 2:
				return 4;
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
				return 1;
			}

			return 0;
		}
	}

	namespace PSConstants
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "TextureProj";
			case 1:return "ShallowColor";
			case 2:return "DeepColor";
			case 3:return "ReflectionColor";
			case 4:return "FresnelRI";
			case 5:return "BlendRadius";
			case 6:return "PosAdjust";
			case 7:return "ReflectPlane";
			case 8:return "CameraData";
			case 9:return "ProjData";
			case 10:return "VarAmounts";
			case 11:return "FogParam";
			case 12:return "FogNearColor";
			case 13:return "FogFarColor";
			case 14:return "SunDir";
			case 15:return "SunColor";
			case 16:return "NumLights";
			case 17:return "LightPos";
			case 18:return "LightColor";
			case 19:return "WaterParams";
			case 20:return "DepthControl";
			case 21:return "SSRParams";
			case 22:return "SSRParams2";
			case 23:return "NormalsAmplitude";
			case 24:return "VPOSOffset";
			}

			return BSSM_PLACEHOLDER;
		}

		static int GetSize(int Index, unsigned int Flags = 0)
		{
			switch (Index)
			{
			case 0:
				return 4;
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
			case 24:
				return 1;
			case 17:
			case 18:
				return 8;
			}

			return 0;
		}
	}

	namespace Samplers
	{
		static const char *GetString(int Index)
		{
			switch (Index)
			{
			case 0:return "ReflectionSampler";
			case 1:return "RefractionSampler";
			case 2:return "DisplacementSampler";
			case 3:return "CubeMapSampler";
			case 4:return "Normals01Sampler";
			case 5:return "Normals02Sampler";
			case 6:return "Normals03Sampler";
			case 7:return "DepthSampler";
			case 8:return "FlowMapSampler";
			case 9:return "FlowMapNormalsSampler";
			case 10:return "SSReflectionSampler";
			case 11:return "RawSSReflectionSampler";
			}

			return BSSM_PLACEHOLDER;
		}
	}

	namespace Techniques
	{
		static void GetString(uint32_t Technique, char *Buffer, size_t BufferSize)
		{
			LONG bits = Technique;

			strcpy_s(Buffer, BufferSize, "");

			if (Technique & 0x1)
				strcat_s(Buffer, BufferSize, "Vc ");
			if (Technique & 0x2)
				strcat_s(Buffer, BufferSize, "NTex ");
			if (Technique & 0x80)
				strcat_s(Buffer, BufferSize, "Va ");
			if (Technique & 0x20)
				strcat_s(Buffer, BufferSize, "Int ");
			if (Technique & 0x40)
				strcat_s(Buffer, BufferSize, "Disp ");
			if (Technique & 4)
			{
				if (!_bittest(&bits, 8))
					strcat_s(Buffer, BufferSize, "Refl ");
				else
					strcat_s(Buffer, BufferSize, "Cube ");
			}
			if (Technique & 0x8)
				strcat_s(Buffer, BufferSize, "Refr ");
			if (Technique & 0x10)
				strcat_s(Buffer, BufferSize, "Dpth ");

			switch ((Technique >> 11) & 0xF)
			{
			case 8:strcat_s(Buffer, BufferSize, "Underwater "); break;
			case 9:strcat_s(Buffer, BufferSize, "LOD "); break;
			case 10:strcat_s(Buffer, BufferSize, "Stencil "); break;
			case 11:strcat_s(Buffer, BufferSize, "Simple "); break;

				// Anything under 8 is actually an index (NUM_SPECULAR_LIGHTS)
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				strcat_s(Buffer, BufferSize, "Specular ");
				break;
			}

			Trim(Buffer, ' ');
		}
	}

	namespace Defines
	{
		static auto GetArray(uint32_t Technique)
		{
			std::vector<std::pair<const char *, const char *>> defines;

			defines.emplace_back("WATER", "");
			defines.emplace_back("FOG", "");

			if (TEST_BIT(0)) defines.emplace_back("VC", "");
			if (TEST_BIT(1)) defines.emplace_back("NORMAL_TEXCOORD", "");
			if (TEST_BIT(2)) defines.emplace_back("REFLECTIONS", "");
			if (TEST_BIT(3)) defines.emplace_back("REFRACTIONS", "");
			if (TEST_BIT(4)) defines.emplace_back("DEPTH", "");
			if (TEST_BIT(5)) defines.emplace_back("INTERIOR", "");
			if (TEST_BIT(6)) defines.emplace_back("WADING", "");
			if (TEST_BIT(7)) defines.emplace_back("VERTEX_ALPHA_DEPTH", "");
			if (TEST_BIT(8)) defines.emplace_back("CUBEMAP", "");
			if (TEST_BIT(9)) defines.emplace_back("FLOWMAP", "");
			if (TEST_BIT(10)) defines.emplace_back("BLEND_NORMALS", "");

			if (((Technique >> 11) & 0xF) < 8)
				defines.emplace_back("SPECULAR", "");

			switch ((Technique >> 11) & 0xF)
			{
			case 8:defines.emplace_back("UNDERWATER", ""); break;
			case 9:defines.emplace_back("LOD", ""); break;
			case 10:defines.emplace_back("STENCIL", ""); break;
			case 11:defines.emplace_back("SIMPLE", ""); break;

			case 0:defines.emplace_back("NUM_SPECULAR_LIGHTS", "0"); break;
			case 1:defines.emplace_back("NUM_SPECULAR_LIGHTS", "1"); break;
			case 2:defines.emplace_back("NUM_SPECULAR_LIGHTS", "2"); break;
			case 3:defines.emplace_back("NUM_SPECULAR_LIGHTS", "3"); break;
			case 4:defines.emplace_back("NUM_SPECULAR_LIGHTS", "4"); break;
			case 5:defines.emplace_back("NUM_SPECULAR_LIGHTS", "5"); break;
			case 6:defines.emplace_back("NUM_SPECULAR_LIGHTS", "6"); break;
			case 7:defines.emplace_back("NUM_SPECULAR_LIGHTS", "7"); break;
			}

			return defines;
		}
	}
}