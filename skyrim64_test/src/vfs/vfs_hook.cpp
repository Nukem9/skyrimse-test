#include "vfs_hook.h"
#include "../../detours/Detours.h"
#include "vfs.h"

__declspec(thread) bool InRecursiveCall = false;

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
    wchar_t *objPath   = nullptr;

    // Build the full path if necessary
    if (Attributes->RootDirectory)
    {
        if (US_StartsWith(Attributes->ObjectName, L"\\??\\") || US_StartsWith(Attributes->ObjectName, L"\\\\?\\") || US_StartsWith(Attributes->ObjectName, L"\\\\.\\") || US_StartsWith(Attributes->ObjectName, L"\\Device\\"))
        {
            // Sometimes a full path and a parent root will be given....I don't know why.....
            goto __fakeroot;
        }

        //
        // We might need to allocate a larger buffer
        //
        // "If the function fails because lpszFilePath is too small
        //  to hold the string plus the terminating null character,
        //  the return value is the required buffer size, in TCHARs.
        //  This value includes the size of the terminating null character."
        //
        DWORD len = GetFinalPathNameByHandleW(Attributes->RootDirectory, nullptr, 0, VOLUME_NAME_DOS);

        if (len == 0)
            return false;
        //throw "Failed GetFinalPathNameByHandleW";

        objPath = new wchar_t[len + nameLength + 2];
        len     = GetFinalPathNameByHandleW(Attributes->RootDirectory, objPath, len, VOLUME_NAME_DOS);

        if (len == 0)
            throw "Failed GetFinalPathNameByHandleW";

        // Append '\' to the last root directory if necessary
        if (Attributes->ObjectName->Buffer[0] != L'\0')
        {
            objPath[len]     = L'\\';
            objPath[len + 1] = L'\0';
        }

        // Append relative directory
        wcsncat_s(objPath, len + nameLength + 2, Attributes->ObjectName->Buffer, nameLength);
    }
    else
    {
    __fakeroot:
        // Non-relative path - do a simple copy
        size_t len = nameLength;
        objPath    = new wchar_t[len + 1];

        memcpy(objPath, Attributes->ObjectName->Buffer, len * sizeof(wchar_t));
        objPath[len] = L'\0';
    }

    wchar_t *data            = objPath;
    vfs::VfsManager *manager = GetManager(&data);

    // 'data' is now the relative virtual path
    if (manager)
    {
		//vfs::BUG_IF(caseSensitive);

        std::string physPath;
        std::wstring physPathW;

        if (!manager->ResolvePhysicalPath(vfs::str::narrow(data), physPath))
        {
            printf("Unable to resolve alias. Dir: %ws\n", objPath);
            delete[] objPath;
            return false;
        }

        physPathW = vfs::str::wide(physPath);

        // Rewrite object attributes
        size_t len = physPathW.length() + 4 + 1; // 4 for "\??\" and 1 for "\0"

		vfs::BUG_IF(len > vfs::NT_MAX_PATH);

        memcpy(Out, Attributes, sizeof(OBJECT_ATTRIBUTES));
        Out->RootDirectory             = nullptr;
        Out->ObjectName                = new UNICODE_STRING;
        Out->ObjectName->Buffer        = new wchar_t[len];
        Out->ObjectName->Length        = (USHORT)((len - 1) * sizeof(wchar_t)); // Excludes null character
        Out->ObjectName->MaximumLength = Out->ObjectName->Length;

        memcpy(&Out->ObjectName->Buffer[0], L"\\??\\", 4 * sizeof(wchar_t));
        memcpy(&Out->ObjectName->Buffer[4], physPathW.c_str(), physPathW.length() * sizeof(wchar_t));
        Out->ObjectName->Buffer[len - 1] = L'\0';

        delete[] objPath;
        return true;
    }

    delete[] objPath;
    return false;
}

decltype(&NtOpenFile) ptrNtOpenFile;
NTSTATUS NTAPI hk_NtOpenFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions)
{
    OBJECT_ATTRIBUTES newAttributes;
    if (!InRecursiveCall && RewriteObjectAttributes(ObjectAttributes, &newAttributes))
    {
        InRecursiveCall = true;

        NTSTATUS ret = ptrNtOpenFile(FileHandle, DesiredAccess, &newAttributes, IoStatusBlock, ShareAccess, OpenOptions);

        delete[] newAttributes.ObjectName->Buffer;
        delete newAttributes.ObjectName;

        InRecursiveCall = false;
        return ret;
    }

    return ptrNtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, ShareAccess, OpenOptions);
}

decltype(&NtCreateFile) ptrNtCreateFile;
NTSTATUS hk_NtCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_ PVOID EaBuffer,
    _In_ ULONG EaLength)
{
    OBJECT_ATTRIBUTES newAttributes;
    if (!InRecursiveCall && RewriteObjectAttributes(ObjectAttributes, &newAttributes))
    {
        InRecursiveCall = true;

        NTSTATUS ret = ptrNtCreateFile(FileHandle, DesiredAccess, &newAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);

        delete[] newAttributes.ObjectName->Buffer;
        delete newAttributes.ObjectName;

        InRecursiveCall = false;
        return ret;
    }

    return ptrNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

void DoHook()
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");

    *(uint8_t **)&ptrNtOpenFile   = Detours::X64::DetourFunction((PBYTE)GetProcAddress(ntdll, "NtOpenFile"), (PBYTE)&hk_NtOpenFile);
    *(uint8_t **)&ptrNtCreateFile = Detours::X64::DetourFunction((PBYTE)GetProcAddress(ntdll, "NtCreateFile"), (PBYTE)&hk_NtCreateFile);
}
