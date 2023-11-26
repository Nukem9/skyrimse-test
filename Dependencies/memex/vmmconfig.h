//
//Copyright(c) 2023 Perchik71 <email:timencevaleksej@gmail.com>
//

#pragma once

#ifndef VOLTEK_LIB_BUILD
#	ifdef VMMDLL_EXPORTS
#		define VOLTEK_MM_API __declspec(dllexport)
#	else
#		define VOLTEK_MM_API __declspec(dllimport)
#	endif // VOLTEK_DLL_BUILD
#else
#	define VOLTEK_MM_API
#endif // !VOLTEK_LIB_BUILD
