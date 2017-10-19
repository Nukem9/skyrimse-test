#include "../stdafx.h"

// clang-format off
struct
{
    const char *Section;
    const char *Key;
    const char *Value;
} const Settings[] =
{
	// '!' means the variable is never used in the engine. It's included here anyway.
	// '?' means used by engine. Descrption not written or use is unknown.

	// Skyrim.ini
	{ "General",	"fDefaultFov",					"90.000" },	//  Field of view
	{ "General",	"fDefaultWorldFOV",				"90.000" },	//  ^
	{ "General",	"fDefault1stPersonFOV",			"90.000" },	//  ^

	{ "General",	"sIntroSequence",				"" },		//  Disable Bethesda intro video
	{ "General",	"bModManagerMenuEnabled",		"0" },		//! Don't show the mod manager on the main menu

	{ "Display",	"fSunShadowUpdateTime",			"1" },		//  Smooth sun shadow transitions
	{ "Display",	"fSunUpdateThreshold",			"0" },		//  http://forum.step-project.com/topic/9209-sun-shadow-transition-ini-settings/

	//
	// Framerate
	//
	{ "General",	"bAlwaysActive",				"1" },		//  Don't pause the game, even if minimized/in the background
	{ "General",	"iFPSClamp",					"0" },		//  Unlock internal FPS limiter #1

	{ "Display",	"iVSyncPresentInterval",		"0" },		//  Disable D3D11::Present() VSync
	{ "Display",	"bLockFramerate",				"0" },		//  Unlock internal FPS limiter #2
	{ "Display",	"bCompensateUnstableFrameTime",	"0" },		//! Disable game tweaks from shader compile hitching
	{ "Display",	"iUnstableFrameTimeHistorySize","1" },		//! ^

	{ "Havok",		"fMaxTime",						"0.0111" },	//  Physics frame time (90 FPS = 1 / 0.0111)
	{ "Havok",		"fMaxTimeComplex",				"0.0111" },	//  Complex physics frame time
	{ "Havok",		"iNumThreads",					"2" },		//! No references

	//
	// Memory / Cache / Preload
	//
	{ "General",	"uGridsToLoad",					"5" },		//  Maximum active cells at once (higher => unstable)
	{ "General",	"uInterior Cell Buffer",		"16" },		//  uGrid interior cell buffer
	{ "General",	"uExterior Cell Buffer",		"128" },		//  uGrid world cell buffer
	{ "General",	"bPreloadIntroSequence",		"0" },		//  We don't care about the intro video any more (sIntroSequence)
	{ "General",	"bPreloadLinkedInteriors",		"0" },		//?
	{ "General",	"bBackgroundLoadVMData",		"0" },		//?
	{ "General",	"bEnableFileCaching",			"0" },		//  Cache files in memory. Causes crash.
	{ "General",	"iNumHWThreads",				"12" },		//! Has one reference, never used otherwise

	{ "Display",	"uLargeRefLODGridSize",			"21" },		//  Maximum high LOD cells to keep active

	{ "BackgroundLoad","bBackgroundCellLoads",		"0" },		//?
	{ "BackgroundLoad","bLoadHelmetsInBackground",	"0" },		//?
	{ "BackgroundLoad","bUseMultiThreadedFaceGen",	"0" },		//?

	{ "Pathfinding","bBackgroundPathing",			"1" },		//?
	{ "Pathfinding","bBackgroundNavmeshUpdate",		"1" },		//?

	{ "Archive",	"bLoadEsmInMemory",				"1" },		//  Cache ESMs in memory
	{ "Archive",	"bLoadArchiveInMemory",			"1" },		//  Cache 'sArchiveToLoadInMemoryList' archives in memory
	{ "Archive",	"bForceAsync",					"0" },		//? Causes crash
	{ "Archive",	"bKeepDLStringBlocksLoaded",	"1" },		//?
	{ "Archive",	"bKeepILStringBlocksLoaded",	"1" },		//?

	{ "Papyrus",	"iMaxAllocatedMemoryBytes",		"8388608" },//  Increase max script memory (8MB)
	{ "Papyrus",	"iMaxMemoryPageSize",			"1024" },	//  Max single allocation size
	{ "Papyrus",	"iMinMemoryPageSize",			"128" },	//  Min single allocation size
	{ "Papyrus",	"fArchiveInitBufferMB",			"8" },		//! Has one reference, nullsub

	{ "Audio",		"uiInitialCacheSize",			"2097152" },//  Was 1MB, now 2MB
	{ "Audio",		"uiMaxAudioCacheSize",			"4194304" },//  Was 2MB, now 4MB
	{ "Audio",		"uMaxSizeForCachedSound",		"524288" },	//  Was 256KB, now 512KB
	{ "Audio",		"uStreamingThreshold",			"5242880" },//  Was 4MB, now 5MB

	{ "Animation",	"bInitiallyLoadAllClips",		"0" },		//  BREAKS ALL ANIMS - don't delay-load animations as needed. Load all up front. (IAnimationClipLoaderSingleton)

	//
	// Gameplay
	//
	{ "General",	"bBorderRegionsEnabled",		"0" },		// Unlock map limits / disable "You cannot go this way"
	{ "General",	"bAnimateDoorPhysics",			"1" },		// ???
	{ "General","bFlyingMountFastTravelCruiseEnabled","1" },	// Fly on a dragon to fast travel destination
	{ "GamePlay","bAllowDragonFlightLocationDiscovery","1" },	// Discover new locations while on said dragon
	{ "GamePlay","bEssentialTakeNoDamage","1" },	// ???

	{ "Interface","bUseAllNonDefaultLoadScreensFirst","1" },	// "Makes the loading screens slightly less boring. Shows less common loading screen items more often."

	{ "Landscape",	"fLandFriction",				"30" },		// More realistic friction (terrain only, excludes roads)

	{ "Havok",		"fInAirFallingCharGravityMult",	"4" },		// More realistic player gravity

	{ "Combat",		"f1PArrowTiltUpAngle",			"0.8" },	// Corrects the arrow tilt which makes arrows and bolts fire higher than they should,
	{ "Combat",		"f1PBoltTiltUpAngle",			"0.4" },	// ...as compared to the relative placement of the aiming reticule.
	{ "Combat",		"f3PArrowTiltUpAngle",			"2.5" },	// Vanilla value - effect unknown.

	//
	// Misc visual tweaks
	//
	{ "Display", "bSAOEnable", "0" },	// Screen Space Ambient Occlusion. Causes ghosting/flickering in SLI mode.

	{ "Display", "bDeactivateAOOnSnow", "0" },
	{ "Display", "bEnableParallaxOcclusion", "1" },
	{ "Display", "bEnableSnowRimLighting", "0" },		// Prevent weird glowing on snowy object edges at night

	{ "Display",		"bEnableDownsampleComputeShader",	"1" }, // ???
	{ "Display",		"bEnableFrontToBackPrepass",		"0" }, // Don't enable. Used if your GPU is significantly weaker than your CPU.
	{ "Display",		"bActorSelfShadowing",				"1" },
	{ "Display",		"bDrawLandShadows",					"1" },
	{ "Imagespace",		"bDoDepthOfField",					"0" },
	{ "Display",		"fDecalLifetime",					"180" },	// Decals last for 3 minutes instead of 30 seconds
	{ "Particles",		"iMaxDesired",						"3000" }, // Bumped up from the 750 default

	{ "General",	"bPreemptivelyUnloadCells",				"1" },

	{ "TerrainManager",	"fBlockLevel0Distance",				"75000.0000" },
	{ "TerrainManager",	"fBlockLevel1Distance",				"140000.0000" },
	{ "TerrainManager",	"fBlockMaximumDistance",			"500000.0000" },
	{ "TerrainManager",	"fSplitDistanceMult",				"1.5000" },

	/*
	{ "Trees",			"bForceFullDetail",					"1" },
	{ "Trees",			"bRenderSkinnedTrees",				"1" },
	{ "Trees",			"uiMaxSkinnedTreesToRender",		"400" },
	*/
		/*
	{ "Grass",			"iMaxGrassTypesPerTexure",			"7" },	// Allows more unique grass types to appear per terrain texture. Bumped from 2.
	{ "Grass",			"fGrassStartFadeDistance",			"195000.0000" },
	{ "Grass",			"fGrassMaxStartFadeDistance",		"200000.0000" },
	{ "Grass",			"fGrassMinStartFadeDistance",		"190000.0000" },
	{ "Grass",			"iGrassCellRadius",					"4" },
	{ "Grass",			"iMinGrassSize",					"1" },
	*/
		/*
	{ "TerrainManager",	"fBlockLevel0Distance",				"70000.0000" },
	{ "TerrainManager",	"fBlockLevel1Distance",				"120000.0000" },
	{ "TerrainManager",	"fBlockMaximumDistance",			"250000.0000" },
	{ "TerrainManager",	"fSplitDistanceMult",				"2.0000" },
	{ "TerrainManager",	"fTreeLoadDistance",				"125000.0000" },
	{ "TerrainManager",	"fSplitDistanceMult",				"10.0000" },
	*/
		/*
	{ "Papyrus",			"bEnableLogging",					"1" },
	{ "Papyrus",			"bLoadDebugInformation",					"1" },
	{ "Papyrus",			"bEnableProfiling",					"1" },
	{ "Papyrus",			"bEnableTrace",					"1" },
	*/
};
// clang-format on

DWORD WINAPI hk_GetPrivateProfileStringA(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize, LPCTSTR lpFileName)
{
    // Check for overrides first
    for (auto &setting : Settings)
    {
        if (!_stricmp(lpAppName, setting.Section) && !_stricmp(lpKeyName, setting.Key))
        {
            strcpy_s(lpReturnedString, nSize, setting.Value);
            return (DWORD)strlen(setting.Value);
        }
    }

    return GetPrivateProfileStringA(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
}

UINT WINAPI hk_GetPrivateProfileIntA(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName)
{
    // Check for overrides first
    for (auto &setting : Settings)
    {
        if (!_stricmp(lpAppName, setting.Section) && !_stricmp(lpKeyName, setting.Key))
            return atoi(setting.Value);
    }

    return GetPrivateProfileIntA(lpAppName, lpKeyName, nDefault, lpFileName);
}

void PatchSettings()
{
    PatchIAT(hk_GetPrivateProfileStringA, "kernel32.dll", "GetPrivateProfileStringA");
    PatchIAT(hk_GetPrivateProfileIntA, "kernel32.dll", "GetPrivateProfileIntA");
}
