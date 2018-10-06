#include "../../common.h"
#include "Console.h"

void Console::ExecuteCommand(const char *Command, ...)
{
	// Field at 0x30 is referenced but never seems to be used
	static char fakeMenuData[0x38] = { 0 };

	char buffer[2048];
	va_list va;

	va_start(va, Command);
	vsnprintf_s(buffer, _TRUNCATE, Command, va);
	va_end(va);

	UIData::Param p;
	memset(&p, 0, sizeof(UIData::Param));
	p.String = buffer;

	UIData data;
	memset(&data, 0, sizeof(UIData));
	data.MenuHandle = (class IMenu *)fakeMenuData;
	data.Parameter = &p;

	UIExecuteCommand(&data);
}

void Console::UIExecuteCommand(UIData *Data)
{
	AutoFunc(void(__fastcall *)(UIData *), sub_14085A2E0, 0x85A2E0);
	sub_14085A2E0(Data);
}