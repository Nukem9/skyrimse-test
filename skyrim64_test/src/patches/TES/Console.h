#pragma once

#include "MemoryContextTracker.h"

class Console
{
public:
	struct UIData
	{
		struct Param
		{
			char _pad[0x10];
			char *String;
		};

		char _pad0[0x8];
		uint32_t Flags;
		char _pad1[0x8];
		class IMenu *MenuHandle;
		char _pad2[0x8];
		Param *Parameter;
	};

	static void ExecuteCommand(const char *Command, ...);
	static void UIExecuteCommand(UIData *Data);
};
static_assert_offset(Console::UIData, Flags, 0x8);
static_assert_offset(Console::UIData, MenuHandle, 0x18);
static_assert_offset(Console::UIData, Parameter, 0x28);