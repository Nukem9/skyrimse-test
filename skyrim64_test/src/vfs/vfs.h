#pragma once

#include "vfs_util.h"
#include <Shlwapi.h>
#include <Windows.h>
#include <regex>
#include <string>

namespace vfs
{
	const size_t NT_MAX_PATH = 32767;

    extern std::vector<class VfsManager *> Managers;

	__forceinline void BUG_IF(bool Condition)
	{
		if (Condition)
			throw "Something went horribly wrong. Bad code.";
	}

    static std::string DriveToDos(const std::string &Drive)
    {
        // "E:\Program Files (x86)\Steam\" -> "E:"
        std::regex driveRegex("([A-z]:)");

        std::smatch matches;
        if (!std::regex_search(Drive, matches, driveRegex))
            return "";

        return util::TrimSlash("\\??\\" + matches[1].str());
    }

    static std::string DriveToDevice(const std::string &Drive)
    {
        // "E:\Program Files (x86)\Steam\" -> "E:"
        std::regex driveRegex("([A-z]:)");

        std::smatch matches;
        if (!std::regex_search(Drive, matches, driveRegex))
            return "";

        wchar_t targetPaths[512];
        if (QueryDosDeviceW(str::wide(matches[1].str()).c_str(), targetPaths, ARRAYSIZE(targetPaths)) == 0)
            return "";

        return util::TrimSlash(str::narrow(targetPaths));
    }

    static std::string DriveToGuid(const std::string &Drive)
    {
        // "E:\Program Files (x86)\Steam\" -> "E:"
        std::regex driveRegex("([A-z]:)");

        std::smatch matches;
        if (!std::regex_search(Drive, matches, driveRegex))
            return "";

        std::wstring volume = str::wide(matches[1].str() + "\\");
        wchar_t volumeGuid[50];
        if (GetVolumeNameForVolumeMountPointW(volume.c_str(), volumeGuid, ARRAYSIZE(volumeGuid)) == 0)
            return "";

        return util::TrimSlash(str::narrow(volumeGuid));
    }

    struct VfsEntry
    {
		struct VfsDirEntry *Parent; // Equivalent to '..' in any filesystem
        std::string Alias;
        std::string Name;
        // std::string ShortName;
        bool IsFile;  // VfsFileEntry or VfsDirEntry
        bool IsAlias; // Equivalent to a symbolic link

        VfsEntry(bool File)
            : IsFile(File)
        {
        }
    };

    struct VfsDirEntry : VfsEntry
    {
        std::vector<VfsEntry *> Listing;

        VfsDirEntry()
            : VfsEntry(false)
        {
        }

        VfsEntry *LinearFind(const std::string &Name, bool Files)
        {
            return LinearFind(Name.c_str(), Files);
        }

        VfsEntry *LinearFind(const char *Name, bool Files)
        {
            for (const auto &entry : this->Listing)
            {
                if (!Files)
                {
                    if (entry->IsFile)
                        continue;
                }

                if (_stricmp(entry->Name.c_str(), Name) != 0)
                    continue;

                return entry;
            }

            return nullptr;
        }
    };

    struct VfsFileEntry : VfsEntry
    {
        VfsFileEntry()
            : VfsEntry(true)
        {
        }
    };

    class VfsManager
    {
    public:
        std::string MountPoint; // E:\\My Virtual Folder
        std::string RealPath;   // E:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special Edition

        std::string RootDos;    // \??\E:
        std::string RootDevice; // \Device\HarddiskVolume1
        std::string RootGuid;   // \Device\{E2E51C7F-E993-4286-A735-8F744F15902D}

        std::wstring RootDosW;
        std::wstring RootDosFullW;
        std::wstring RootDeviceW;
        std::wstring RootDeviceFullW;
        std::wstring RootGuidW;
        std::wstring RootGuidFullW;

        VfsDirEntry r;

        VfsManager()
        {
        }
        VfsManager(const VfsManager &) = delete;

		bool ResolvePhysicalPath(std::string &VirtPath, std::string &RealPath);
		VfsEntry *Resolve(std::string &VirtPath);
		VfsDirEntry *ResolveDir(std::string &VirtPath);
		VfsFileEntry *ResolveFile(std::string &VirtPath);
    public:
		bool CreateFileAlias(std::string VirtPath, std::string RealPath);

		/// <summary><para>
		/// Create an empty folder in memory. This folder doesn't exist anywhere on
		/// a hard drive.
		/// </para></summary>
		bool CreateDirectory(std::string VirtPath);

		/// <summary><para>
		/// Create a folder in memory that allows pass-thru to an on-disk directory. This
		/// is effectively the same as MountVirtualDirectory, but within a specific VirtPath.
		/// </para>
		/// <para>Note:</para>
		/// <para>The parent virtual folder must exist already (via CreateDirectory/CreateDirectoryAlias).</para>
		/// </summary>
		bool CreateDirectoryAlias(std::string VirtPath, std::string RealPath);

		/// <summary><para>
		/// Create a new virtual filesystem manager.
		/// </para>
		/// <para>Note:</para>
		/// <para> - RealPath and VirtPath must be a directory. File paths are not valid.</para>
		/// <para> - RealPath is required when AllowPassthru = true.</para>
		/// <para> - RealPath can be ex. "E:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition"
		/// and wouldn't be emulating any file paths initially. VirtPath would be the symlink.</para>
		/// <para> - VirtPath can be a fake path and doesn't need to "exist" on the hard drive. However, it must be
		/// formatted correctly.</para>
		/// </summary>
		static VfsManager *MountVirtualDirectory(std::string VirtPath, std::string RealPath = "", bool AllowPassthru = false);
    };
}
