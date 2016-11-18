#define WIN32_LEAN_AND_MEAN
#define CINTERFACE
#define DIRECTINPUT_VERSION 0x800
#include <Windows.h>
#include <dinput.h>
#include <vector>
#include <unordered_map>
#include "util.h"

#include "detours/Detours.h"

// DirectInput 8.0 ANSI
namespace IDirectInput8AHook
{
	static_assert(offsetof(IDirectInput8A, lpVtbl) == 0, "VTable offset not at class base");
	static_assert(offsetof(IDirectInputDevice8A, lpVtbl) == 0, "VTable offset not at class base");

	IDirectInput8AVtbl vtableBackup;
	IDirectInputDevice8AVtbl deviceVtableBackup;

	bool vtableInit = false;
	bool deviceVtableInit = false;

	//
	// Container for all device instances:
	// True if it is a keyboard
	// False if it is not (mouse, ...)
	//
	std::unordered_map<IDirectInputDevice8A *, bool> keyboardTypes;

	HRESULT WINAPI SetCooperativeLevel(IDirectInputDevice8A *thisptr, HWND hwnd, DWORD dwFlags)
	{
		auto entry = keyboardTypes.find(thisptr);

		if (entry != keyboardTypes.end() && entry->second)
			dwFlags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

		return deviceVtableBackup.SetCooperativeLevel(thisptr, hwnd, dwFlags);
	}

	HRESULT WINAPI CreateDevice(IDirectInput8A *thisptr, REFGUID rguid, IDirectInputDevice8A **lplpDirectInputDevice, IUnknown *pUnkOuter)
	{
		HRESULT hr = vtableBackup.CreateDevice(thisptr, rguid, lplpDirectInputDevice, pUnkOuter);

		// Exclusive:    GUID_SysMouse
		// Nonexclusive: GUID_SysKeyboard
		keyboardTypes[*lplpDirectInputDevice] = false;

		if (SUCCEEDED(hr) && rguid == GUID_SysKeyboard)
		{
			keyboardTypes[*lplpDirectInputDevice] = true;
			auto device = *lplpDirectInputDevice;

			// Get a copy of the original ANSI vtable
			if (!deviceVtableInit)
			{
				deviceVtableInit = true;
				memcpy(&deviceVtableBackup, device->lpVtbl, sizeof(IDirectInputDevice8AVtbl));
			}

			// Replace the vtable pointer globally
			ULONG_PTR funcPtr		= (ULONG_PTR)&SetCooperativeLevel;
			ULONG_PTR vtableBase	= (ULONG_PTR)device->lpVtbl;

			PatchMemory(vtableBase + offsetof(IDirectInputDevice8AVtbl, SetCooperativeLevel), (PBYTE)&funcPtr, sizeof(ULONG_PTR));
		}

		return hr;
	}

	void Apply(IDirectInput8A *DirectInput)
	{
		// Get a copy of the original ANSI vtable
		if (!vtableInit)
		{
			vtableInit = true;
			memcpy(&vtableBackup, DirectInput->lpVtbl, sizeof(IDirectInput8AVtbl));
		}

		// Replace the vtable pointer globally
		ULONG_PTR funcPtr		= (ULONG_PTR)&CreateDevice;
		ULONG_PTR vtableBase	= (ULONG_PTR)DirectInput->lpVtbl;

		PatchMemory(vtableBase + offsetof(IDirectInput8AVtbl, CreateDevice), (PBYTE)&funcPtr, sizeof(ULONG_PTR));
	}
}

// DirectInput 8.0 Unicode
namespace IDirectInput8WHook
{
	void Apply(IDirectInput8W *DirectInput)
	{
		MessageBoxA(nullptr, "UNICODE UNIMPLEMENTED", "???", 0);
	}
}

HRESULT WINAPI hk_DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, IUnknown *punkOuter)
{
	HRESULT hr = DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	if (SUCCEEDED(hr))
	{
		if (riidltf == IID_IDirectInput8A)
			IDirectInput8AHook::Apply(reinterpret_cast<IDirectInput8A *>(*ppvOut));
		else if (riidltf == IID_IDirectInput8W)
			IDirectInput8WHook::Apply(reinterpret_cast<IDirectInput8W *>(*ppvOut));
		else
			MessageBoxA(nullptr, "UNEXPECTED GUID", "???", 0);
	}

	return hr;
}

void PatchDInput()
{
	Detours::X64::DetourIAT((PBYTE)GetModuleHandleA(nullptr), (PBYTE)hk_DirectInput8Create, "dinput8.dll", "DirectInput8Create");
}