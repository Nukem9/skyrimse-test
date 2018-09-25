#pragma once

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#ifndef SKYRIM64_CREATIONKIT_DLL
#define SKYRIM64_CREATIONKIT_DLL	0	// Build for replacing d3d9.dll instead of d3dx9_42.dll (Also skips game patch code)
#endif

#define SKYRIM64_D3DX9_DEBUG		0	// Enable debugging checks in d3dx9_42 overrides
#define SKYRIM64_GENERATE_OFFSETS	0	// Dump offset list to disk in codegen.cpp
#define SKYRIM64_USE_VTUNE			0	// Enable VTune instrumentation API
#define SKYRIM64_USE_VFS			0	// Enable virtual file system
#define SKYRIM64_USE_PROFILER		1	// Enable built-in profiler macros / "profiler.h"
#define SKYRIM64_USE_TRACY			1	// Enable tracy client + server / https://bitbucket.org/wolfpld/tracy/overview
#define SKYRIM64_USE_TBBMALLOC		1	// Use Intel's TBB allocator instead of jemalloc