#include "../common.h"
#include "vfs.h"
#include "vfs_hook.h"

// Some other WinAPI functions are necessary, but they use our hooked code path
__declspec(thread) bool InRecursiveCall;

// Declare these as per-thread to prevent allocating/freeing buffers over and over (RewriteObjectAttributes)
__declspec(thread) UNICODE_STRING ThreadStringAttr;
__declspec(thread) wchar_t ThreadStringAttrBuffer[vfs::NT_MAX_PATH + 1];
__declspec(thread) wchar_t ThreadObjPath[vfs::NT_MAX_PATH + 1];

#define StartsWith(x, y) (_wcsnicmp((x), (y), wcslen(y)) == 0)
#define US_StartsWith(x, y) (((x)->Length / sizeof(wchar_t)) >= wcslen(y) && _wcsnicmp(((x)->Buffer), (y), wcslen(y)) == 0)

void once()
{
    static bool initonce = false;

    if (!initonce)
    {
        initonce = true;

        using namespace vfs;

        VfsManager *test = VfsManager::MountVirtualDirectory("E:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special Edition\\", "E:\\cygwin64\\home\\Administrator", true);

        test->CreateDirectoryA("\\ADir1\\");

        test->CreateDirectoryA("Lol");
        test->CreateDirectoryA("Lol\\Directory2");
        test->CreateDirectoryA("Lol\\Directory2\\Directory3");

        test->CreateDirectoryA("\\Lol2");
        test->CreateDirectoryA("\\Lol2\\Directory2");
        test->CreateDirectoryA("\\Lol2\\Directory2\\Directory3");
        test->CreateDirectoryA("\\Lol2\\Directory2\\Directory3");
        test->CreateDirectoryA("Lol2\\Directory2");

        test->CreateDirectoryAlias("\\data", R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\data\)");
        test->CreateDirectoryAlias("\\hrtf", R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\hrtf\)");

		test->CreateDirectoryAlias("\\enbseries",		R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\enbseries\)");
		test->CreateFileAlias("enbadaptation.fx",		R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\enbadaptation.fx)");
		test->CreateFileAlias("enbbloom.fx",			R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\enbbloom.fx)");
		test->CreateFileAlias("enbdepthoffield.fx",		R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\enbdepthoffield.fx)");
		test->CreateFileAlias("enbeffect.fx",			R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\enbeffect.fx)");
		test->CreateFileAlias("enbeffectpostpass.fx",	R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\enbeffectpostpass.fx)");
		test->CreateFileAlias("enblens.fx",				R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\enblens.fx)");
		test->CreateFileAlias("enblocal.ini",			R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\enblocal.ini)");
		test->CreateFileAlias("enbseries.ini",			R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\enbseries.ini)");
		test->CreateFileAlias("license_en.txt",			R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\license_en.txt)");
		test->CreateFileAlias("readme_en.txt",			R"(E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\test\readme_en.txt)");

        fflush(stdout);
    }
}

vfs::VfsManager *GetManager_Dos(wchar_t **VirtualPart)
{
	if (StartsWith(*VirtualPart, L"\\??\\") ||
		StartsWith(*VirtualPart, L"\\\\?\\") ||
		StartsWith(*VirtualPart, L"\\\\.\\"))
	{
		*VirtualPart += 4;
		//vfs::BUG_IF(wcschr(*VirtualPart, L':') == nullptr);

		for (vfs::VfsManager *m : vfs::Managers)
		{
			const wchar_t *ptr = m->RootDosFullW.c_str() + 4;
			const size_t len = m->RootDosFullW.length() - 4;

			if (_wcsnicmp(*VirtualPart, ptr, len) != 0)
				continue;

			*VirtualPart += len;
			return m;
		}
	}

	return nullptr;
}

vfs::VfsManager *GetManager_Nt(wchar_t **VirtualPart)
{
	if (StartsWith(*VirtualPart, L"\\Device\\"))
	{
		vfs::BUG_IF(wcschr(*VirtualPart, L':') != nullptr);

		for (vfs::VfsManager *m : vfs::Managers)
		{
			const wchar_t *ptr = m->RootDeviceFullW.c_str();
			const size_t len = m->RootDeviceFullW.length();

			if (_wcsnicmp(*VirtualPart, ptr, len) != 0)
				continue;

			*VirtualPart += len;
			return m;
		}
	}

	return nullptr;
}

vfs::VfsManager *GetManager(wchar_t **VirtualPart)
{
	vfs::VfsManager *m;

	if ((m = GetManager_Dos(VirtualPart)) != nullptr)
		return m;

	if ((m = GetManager_Nt(VirtualPart)) != nullptr)
		return m;

    /*else if (StartsWith(*VirtualPart, L"-- GUID --"))
	{
	}*/

    return nullptr;
}

bool RewriteObjectAttributes(POBJECT_ATTRIBUTES Attributes, POBJECT_ATTRIBUTES Out)
{
    //
    // NOTE: Relative paths can still be used here!! (Ex: "hrtf\\*.mhr")
    //
    // Object manager paths:
    // \\?\        -- Disable any WinAPI name expansion. Allows the use of ".." and "."
    // \??\        -- Symbolic link to DosDevices.
    // \\.\        -- Win32 device namespace. Does *not* attempt to locate a predefined alias.
    // \Device\    -- Win32 device namespace.
    //
    if (!Attributes->ObjectName->Buffer || Attributes->ObjectName->Length <= 0)
        return false;

    size_t nameLength  = (Attributes->ObjectName->Length / sizeof(WCHAR));
    bool caseSensitive = (Attributes->Attributes & OBJ_CASE_INSENSITIVE) == 0;

	vfs::BUG_IF(nameLength > vfs::NT_MAX_PATH);

    // if (RootDirectory) => Combine RootDirectory+ObjectName
    if (Attributes->RootDirectory)
    {
        if (US_StartsWith(Attributes->ObjectName, L"\\??\\") || US_StartsWith(Attributes->ObjectName, L"\\\\?\\") || US_StartsWith(Attributes->ObjectName, L"\\\\.\\") || US_StartsWith(Attributes->ObjectName, L"\\Device\\"))
        {
            // Sometimes a full path and a parent root will be given....I don't know why.....
            goto __fakeroot;
        }

		// This call can never fail due to a buffer size as it exceeds NTFS maximum length. However,
		// if it's not a DOS path, it will fail.
        DWORD len = GetFinalPathNameByHandleW(Attributes->RootDirectory, ThreadObjPath, ARRAYSIZE(ThreadObjPath), VOLUME_NAME_DOS);

		if (len == 0)
			return false;

        // Append '\' to the last root directory
        if (Attributes->ObjectName->Buffer[0] != L'\0')
        {
			ThreadObjPath[len]     = L'\\';
			ThreadObjPath[len + 1] = L'\0';
        }

        // Append relative directory
        wcsncat_s(ThreadObjPath, ARRAYSIZE(ThreadObjPath), Attributes->ObjectName->Buffer, nameLength);
    }
    else
    {
    __fakeroot:
        // Absolute path - do a simple copy
        memcpy(ThreadObjPath, Attributes->ObjectName->Buffer, nameLength * sizeof(wchar_t));
		ThreadObjPath[nameLength] = L'\0';
    }

    wchar_t *relativeVirtPath = ThreadObjPath;
    vfs::VfsManager *manager  = GetManager(&relativeVirtPath);

    if (manager)
    {
		//vfs::BUG_IF(caseSensitive);

        std::string physPath;
        std::wstring physPathW;

        if (!manager->ResolvePhysicalPath(vfs::str::narrow(relativeVirtPath), physPath))
        {
            printf("Unable to resolve alias. Dir: %ws\n", ThreadObjPath);
            return false;
        }

        physPathW = vfs::str::wide(physPath);

        // Rewrite object attributes
        size_t len = physPathW.length() + 4 + 1; // 4 for "\??\" and 1 for "\0"

		vfs::BUG_IF(len > vfs::NT_MAX_PATH);

        memcpy(Out, Attributes, sizeof(OBJECT_ATTRIBUTES));
        Out->RootDirectory             = nullptr;
        Out->ObjectName                = &ThreadStringAttr;
        Out->ObjectName->Buffer        = ThreadStringAttrBuffer;
        Out->ObjectName->Length        = (USHORT)((len - 1) * sizeof(wchar_t)); // Excludes null character
        Out->ObjectName->MaximumLength = Out->ObjectName->Length;

        memcpy(&Out->ObjectName->Buffer[0], L"\\??\\", 4 * sizeof(wchar_t));
        memcpy(&Out->ObjectName->Buffer[4], physPathW.c_str(), physPathW.length() * sizeof(wchar_t));
        Out->ObjectName->Buffer[len - 1] = L'\0';
        return true;
    }

    return false;
}

void RewriteObjectAttributesSafe(POBJECT_ATTRIBUTES *Attributes, POBJECT_ATTRIBUTES TempBuffer)
{
	if (!InRecursiveCall)
	{
		InRecursiveCall = true;

		if (RewriteObjectAttributes(*Attributes, TempBuffer))
			*Attributes = TempBuffer;

		InRecursiveCall = false;
	}
}

#define REWRITE_ATTRIBUTES(attrs) OBJECT_ATTRIBUTES newAttributes; RewriteObjectAttributesSafe((attrs), &newAttributes);

decltype(&NtCreateFile) ptrNtCreateFile;
NTSTATUS hk_NtCreateFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength)
{
	REWRITE_ATTRIBUTES(&ObjectAttributes);
	return ptrNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

decltype(&NtDeleteFile) ptrNtDeleteFile;
NTSTATUS NTAPI hk_NtDeleteFile(
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	REWRITE_ATTRIBUTES(&ObjectAttributes);
	return ptrNtDeleteFile(ObjectAttributes);
}

decltype(&NtOpenFile) ptrNtOpenFile;
NTSTATUS NTAPI hk_NtOpenFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG ShareAccess,
	ULONG OpenOptions)
{
	REWRITE_ATTRIBUTES(&ObjectAttributes);
    return ptrNtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

decltype(&NtQueryAttributesFile) ptrNtQueryAttributesFile;
NTSTATUS NTAPI hk_NtQueryAttributesFile(
	POBJECT_ATTRIBUTES ObjectAttributes,
	PFILE_BASIC_INFORMATION FileInformation)
{
	REWRITE_ATTRIBUTES(&ObjectAttributes);
	return ptrNtQueryAttributesFile(ObjectAttributes, FileInformation);
}

decltype(&NtQueryFullAttributesFile) ptrNtQueryFullAttributesFile;
NTSTATUS NTAPI hk_NtQueryFullAttributesFile(
	POBJECT_ATTRIBUTES ObjectAttributes,
	PFILE_NETWORK_OPEN_INFORMATION FileInformation)
{
	REWRITE_ATTRIBUTES(&ObjectAttributes);
	return ptrNtQueryFullAttributesFile(ObjectAttributes, FileInformation);
}

void DoHook()
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");

	*(uint8_t **)&ptrNtCreateFile = Detours::X64::DetourFunction((PBYTE)GetProcAddress(ntdll, "NtCreateFile"), (PBYTE)&hk_NtCreateFile);
	*(uint8_t **)&ptrNtDeleteFile = Detours::X64::DetourFunction((PBYTE)GetProcAddress(ntdll, "NtDeleteFile"), (PBYTE)&hk_NtDeleteFile);
	*(uint8_t **)&ptrNtOpenFile = Detours::X64::DetourFunction((PBYTE)GetProcAddress(ntdll, "NtOpenFile"), (PBYTE)&hk_NtOpenFile);
	*(uint8_t **)&ptrNtQueryAttributesFile = Detours::X64::DetourFunction((PBYTE)GetProcAddress(ntdll, "NtQueryAttributesFile"), (PBYTE)&hk_NtQueryAttributesFile);
	*(uint8_t **)&ptrNtQueryFullAttributesFile = Detours::X64::DetourFunction((PBYTE)GetProcAddress(ntdll, "NtQueryFullAttributesFile"), (PBYTE)&hk_NtQueryFullAttributesFile);
}
