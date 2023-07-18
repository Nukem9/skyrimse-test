#pragma warning(disable : 26451)

#include "..\..\common.h"

#include <psapi.h>
#include <tlhelp32.h>

LPSTR* __DlAllowedPathForDllLoader = NULL;
SIZE_T __DlAllowedPathForDllLoaderCount = 0;

typedef struct MODULEINFOA {
	HMODULE Handle;
	std::string FileName;
	bool HasSystem;
} *PMODULEINFOA;

enum EDllRecoveredFlag : INT32 {
	drfRecoveredFailed = -2,
	drfRecoveredNoModule = -1,
	drfRecoveredOk = 0,
};

class DlMemoryWorker {
public:
	__forceinline SIZE_T StrLen(LPCSTR szStr) const {
		LPCSTR szEnd = szStr;
		while (*szEnd++) {}
		return szEnd - szStr - 1;
	}

	__forceinline INT StrICmp(LPCVOID pv1, LPCVOID pv2, SIZE_T cb) const {
		if (!cb)
			return 0;

		while (--cb && (*(LPSTR)pv1 == *(LPSTR)pv2 || (*(PCHAR)pv1 - *(PCHAR)pv2) == -32) || *(PCHAR)pv1 - *(PCHAR)pv2 == 32) {
			pv1 = (LPSTR)pv1 + 1;
			pv2 = (LPSTR)pv2 + 1;
		}

		return *((PBYTE)pv1) - *((PBYTE)pv2);
	}

	__forceinline INT MemCmp(LPCVOID pv1, LPCVOID pv2, SIZE_T cb) const {
		if (!cb)
			return 0;

		while (--cb && *(LPSTR)pv1 == *(LPSTR)pv2) {
			pv1 = (LPSTR)pv1 + 1;
			pv2 = (LPSTR)pv2 + 1;
		}

		return *((PBYTE)pv1) - *((PBYTE)pv2);
	}

	__forceinline LPVOID MemCpy(LPVOID pvDst, LPCVOID pvSrc, SIZE_T cb) const {
		LPVOID pvRet = pvDst;

		// copy from lower addresses to higher addresses
		while (cb--) {
			*(LPSTR)pvDst = *(LPSTR)pvSrc;
			pvDst = (LPSTR)pvDst + 1;
			pvSrc = (LPSTR)pvSrc + 1;
		}

		return pvRet;
	}

	__forceinline LPCSTR StrCat(LPSTR Dest, LPCSTR Add) const {
		auto to = Dest;
		auto from = Add;

		while (*to) to++;
		for (; *from; to++, from++)
			*to = *from;

		return Dest;
	}

};

class DlImageProcessWorker {
private:
	UINT_PTR Base;
	DlMemoryWorker MemoryWorker;
private:
	__forceinline SIZE_T RFindDelimiter(LPCSTR szStr) const {
		auto len = MemoryWorker.StrLen(szStr) - 1;

		for (; len > 0; len--) {
			if (szStr[len] == '\\' || szStr[len] == '/')
				break;
		}

		len--;
		return len;
	}
public:
	typedef IMAGE_THUNK_DATA* PImgThunkData;
	typedef const IMAGE_THUNK_DATA* PCImgThunkData;
	typedef DWORD RVA;
public:
	enum EDllLoadedFlag : INT32 {
		dlfLoadedFailed = -2,
		dlfLoadedNoLoaded = -1,
		dlfLoadedFromSystemDirectory = 0,
		dlfLoadedFromAllowedDirectory,
		dlfLoadedFromApplicationDirectory,
		dlfLoadedFromOtherDirectory,
	};
public:
	template<class X>
	__forceinline X PFromRva(RVA rva) const { return X(UINT_PTR(Base) + rva); }
	__forceinline VOID SetBase(UINT_PTR base) { Base = base; }
	__forceinline DWORD CountOfImports(PCImgThunkData pitdBase) const {
		DWORD cRet = 0;
		PCImgThunkData pitd = pitdBase;
		while (pitd->u1.Function) {
			pitd++;
			cRet++;
		}
		return cRet;
	}
	__forceinline DWORD CountOfExports(PIMAGE_EXPORT_DIRECTORY pied) const {
		return pied->NumberOfNames;
	}
	__forceinline DWORD IndexFromPImgThunkData(PCImgThunkData pitdCur, PCImgThunkData pitdBase) const {
		return (DWORD)(pitdCur - pitdBase);
	}
	__forceinline PIMAGE_NT_HEADERS64 PinhFromImageBase(HMODULE hmod) const {
		return PIMAGE_NT_HEADERS64(PBYTE(hmod) + PIMAGE_DOS_HEADER(hmod)->e_lfanew);
	}
	__forceinline VOID OverlayIAT(PImgThunkData pitdDst, PCImgThunkData pitdSrc) const {
		MemoryWorker.MemCpy(pitdDst, pitdSrc, CountOfImports(pitdDst) * sizeof IMAGE_THUNK_DATA);
	}
	__forceinline DWORD TimeStampOfImage(PIMAGE_NT_HEADERS pinh) const {
		return pinh->FileHeader.TimeDateStamp;
	}
	__forceinline BOOL FLoadedAtPreferredAddress(PIMAGE_NT_HEADERS pinh, HMODULE hmod) const {
		return UINT_PTR(hmod) == pinh->OptionalHeader.ImageBase;
	}
	EDllLoadedFlag GetDllLoadedFlag(LPCSTR szDllName) const;
	PIMAGE_IMPORT_DESCRIPTOR PiidFromDllName(LPCSTR szDllName) const;
	PIMAGE_EXPORT_DIRECTORY PiedFromDllHandle(HMODULE handle) const;
public:
	DlImageProcessWorker(UINT_PTR base = (UINT_PTR)&__ImageBase);
};

DlImageProcessWorker::DlImageProcessWorker(UINT_PTR base) :
	Base(base)
{}

DlImageProcessWorker::EDllLoadedFlag DlImageProcessWorker::GetDllLoadedFlag(LPCSTR szDllName) const {
	EDllLoadedFlag dlfRet = dlfLoadedFailed;

	CHAR szBuffer[MAX_PATH];
	CHAR szPath[MAX_PATH];

	auto hProcess = GetCurrentProcess();
	if (!hProcess)
		return dlfRet;

	if (!GetSystemDirectoryA(szPath, MAX_PATH))
		return dlfRet;

	auto moduleHandle = GetModuleHandleA(szDllName);
	if (!moduleHandle)
		return dlfLoadedNoLoaded;

	if (!GetModuleFileNameExA(hProcess, moduleHandle, szBuffer, MAX_PATH))
		return dlfRet;

	auto len1 = RFindDelimiter(szPath);
	auto len2 = RFindDelimiter(szBuffer);

	if (len2 == len1) {
		// no system dir
	no_sysdir:
		if (!GetModuleFileNameA(GetModuleHandleA(NULL), szPath, MAX_PATH))
			return dlfRet;

		len1 = RFindDelimiter(szPath);

		if (len1 == len2 && !MemoryWorker.StrICmp(szPath, szBuffer, len1)) {
			dlfRet = dlfLoadedFromApplicationDirectory;
			goto Ret;
		}

		if (__DlAllowedPathForDllLoader && __DlAllowedPathForDllLoaderCount > 0) {
			for (SIZE_T i = 0; i < __DlAllowedPathForDllLoaderCount; i++) {
				auto path = __DlAllowedPathForDllLoader[i];
				len1 = RFindDelimiter(path);

				if (len1 == len2 && !MemoryWorker.StrICmp(szPath, szBuffer, len1)) {
					dlfRet = dlfLoadedFromAllowedDirectory;
					goto Ret;
				}
			}
		}

		dlfRet = dlfLoadedFromOtherDirectory;
	}
	else {
		if (!MemoryWorker.StrICmp(szPath, szBuffer, len1))
			dlfRet = dlfLoadedFromSystemDirectory;
		else
			goto no_sysdir;
	}

Ret:
	return dlfRet;
}

PIMAGE_EXPORT_DIRECTORY DlImageProcessWorker::PiedFromDllHandle(HMODULE handle) const {
	auto pinh = PinhFromImageBase(handle);

	auto sectionExportRVA = pinh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	auto sectionExportSize = pinh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	if (!sectionExportSize)
		return NULL;

	return PFromRva<PIMAGE_EXPORT_DIRECTORY>(sectionExportRVA);
}

PIMAGE_IMPORT_DESCRIPTOR DlImageProcessWorker::PiidFromDllName(LPCSTR szDllName) const {
	PIMAGE_IMPORT_DESCRIPTOR piidRet = NULL;
	auto pinh = PinhFromImageBase(HMODULE(Base));

	auto len = MemoryWorker.StrLen(szDllName);
	if (!len)
		return piidRet;

	auto sectionImportRVA = pinh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	auto sectionImportSize = pinh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;

	// Scan the import for the dll in question
	//
	if (sectionImportSize != 0) {
		PIMAGE_IMPORT_DESCRIPTOR piid = PFromRva<PIMAGE_IMPORT_DESCRIPTOR>(sectionImportRVA);

		// Check all of the dlls listed up to the NULL one.
		//
		for (; piid->Name != NULL; piid++) {
			// Check to see if it is the DLL we want
			// Intentionally case sensitive to avoid complication of using the CRT
			// for those that don't use the CRT...the user can replace this with
			// a variant of a case insensitive comparison routine.
			//
			LPCSTR libName = PFromRva<LPCSTR>(piid->Name);

			SIZE_T cchDllCur = MemoryWorker.StrLen(libName);
			if (cchDllCur == len && MemoryWorker.StrICmp(szDllName, libName, cchDllCur) == 0) {
				piidRet = piid;
				break;
			}
		}
	}

	return piidRet;
}



void ClearModuleList(std::vector<PMODULEINFOA>& List) {
	for (std::size_t i = 0; i < List.size(); i++)
		delete List[i];

	List.clear();
}

bool GetModuleList(HANDLE hProcess, std::vector<PMODULEINFOA>& List) {
	if (!hProcess)
		return false;

	ClearModuleList(List);

	CHAR szModName[MAX_PATH];
	HMODULE hMods[1024];
	DWORD cbNeeded;
	unsigned int i;

	std::string SystemDirectory(MAX_PATH, '\0');
	SystemDirectory.resize(GetSystemDirectoryA(&SystemDirectory[0], MAX_PATH) + 1);
	
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
		auto slen = SystemDirectory.length();

		for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {	
			// Get the full path to the module's file.
			if (GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(CHAR))) {
				auto Info = new MODULEINFOA;
				if (!Info)
					return false;

				Info->FileName = szModName;
				Info->Handle = hMods[i];
				Info->HasSystem = !_stricmp(Info->FileName.substr(0, slen - 1).c_str(), SystemDirectory.c_str());

				List.emplace_back(Info);
			}
		}
	}

	return true;
}

PMODULEINFOA GetModuleInfoByName(std::string LibraryName, std::vector<PMODULEINFOA>& List) {
	for (std::size_t i = 0; i < List.size(); i++) {
		auto Info = List[i];

		auto FName = Info->FileName;
		auto EndIt = FName.find_last_of("\\/");
		if (EndIt != std::string::npos)
			FName = FName.substr(EndIt + 1);

		if (!_stricmp(FName.c_str(), LibraryName.c_str()))
			return Info;
	}


	return nullptr;
}

void Recovery_replacer(DlImageProcessWorker& ImageProcessWorker, DlImageProcessWorker& ImageProcessWorker2, 
	PIMAGE_IMPORT_DESCRIPTOR piid, HMODULE hNewLibrary) {	
	auto hCurrentModule = GetModuleHandleA(NULL);
	auto pinh = ImageProcessWorker.PinhFromImageBase(hCurrentModule);
	auto importProcessRVA = pinh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	auto importProcessSize = pinh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
	auto importProcessAddr = ImageProcessWorker.PFromRva<LPVOID>((uintptr_t)importProcessRVA - 0x1000);

	DWORD dwProtection;
	Assert(VirtualProtect(importProcessAddr, importProcessSize, PAGE_READWRITE, &dwProtection));

	//Import Lookup Table address (functions names)
	auto nameAddressPtr = ImageProcessWorker.PFromRva<PIMAGE_THUNK_DATA>(piid->OriginalFirstThunk);
	//Import Address Table (IAT) address (functions addresses)
	auto functionAddressPtr = ImageProcessWorker.PFromRva<PIMAGE_THUNK_DATA>(piid->FirstThunk);

	for (; nameAddressPtr->u1.Function; nameAddressPtr++, functionAddressPtr++) {
		auto nameFunction = ImageProcessWorker.PFromRva<PIMAGE_IMPORT_BY_NAME>(nameAddressPtr->u1.AddressOfData);		
		auto Func = GetProcAddress(hNewLibrary, nameFunction->Name);
		Assert(Func);
		functionAddressPtr->u1.Function = (DWORD_PTR)Func;
	}

	FlushInstructionCache(GetCurrentProcess(), importProcessAddr, importProcessSize);
	Assert(VirtualProtect(importProcessAddr, importProcessSize, dwProtection, &dwProtection));
}

INT32 Recovery(HMODULE hNewLibrary, LPCSTR szDllName, HMODULE hmod) {
	MODULEENTRY32 modEntry;
	
	DlImageProcessWorker ImageProcessWorker;
	DlImageProcessWorker ImageProcessWorker2;
	UINT_PTR ModuleBase = (UINT_PTR)GetModuleHandle(NULL);

	ImageProcessWorker.SetBase(ModuleBase);
	ImageProcessWorker2.SetBase((UINT_PTR)hNewLibrary);

	auto piid = ImageProcessWorker.PiidFromDllName(szDllName);
	if (piid) Recovery_replacer(ImageProcessWorker, ImageProcessWorker2, piid, hNewLibrary);
	
	auto hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (!hSnap)
		return drfRecoveredFailed;

	modEntry.dwSize = sizeof(MODULEENTRY32);
	if (Module32First(hSnap, &modEntry)) {
		do {
			if ((uintptr_t)modEntry.hModule == ModuleBase || modEntry.hModule == hNewLibrary ||
				modEntry.hModule == (HMODULE)g_hModule || modEntry.hModule == hmod)
				continue;

			ImageProcessWorker.SetBase((UINT_PTR)modEntry.hModule);

			auto pinhOther = ImageProcessWorker.PinhFromImageBase((HMODULE)ModuleBase);
			auto importOtherSize = pinhOther->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
			if (!importOtherSize)
				continue;

			piid = ImageProcessWorker.PiidFromDllName(szDllName);
			if (!piid)
				continue;

			Recovery_replacer(ImageProcessWorker, ImageProcessWorker2, piid, hNewLibrary);
		} while (Module32Next(hSnap, &modEntry));
	}

	CloseHandle(hSnap);

	return drfRecoveredOk;
}

void MaybeNeedRecoveryLibrary(const char* LibName, std::vector<PMODULEINFOA>& ModuleList)
{
	auto Info = GetModuleInfoByName(LibName, ModuleList);
	if (Info && !Info->HasSystem) {
		std::string SystemDirectory(MAX_PATH, '\0');
		SystemDirectory.resize(GetSystemDirectoryA(&SystemDirectory[0], MAX_PATH));
		// Detect lib loaded no from system folder

		auto DirectXLibrary = LoadLibraryA((SystemDirectory + "\\" + LibName).c_str());
		AssertMsgVa(DirectXLibrary, "Library \"%s\" not found in system directory.", LibName);
		Recovery(DirectXLibrary, LibName, Info->Handle);
	}
}

void Disable_ENB() {
	std::vector<PMODULEINFOA> ModuleList;
	AssertMsg(GetModuleList(GetCurrentProcess(), ModuleList), 
		"Failed to get list of libraries by process.");

	MaybeNeedRecoveryLibrary("d3d9.dll", ModuleList);
	MaybeNeedRecoveryLibrary("d3d11.dll", ModuleList);
	MaybeNeedRecoveryLibrary("dxgi.dll", ModuleList);
	
	ClearModuleList(ModuleList);
}