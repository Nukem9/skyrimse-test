#include "vfs_util.h"
#include "vfs.h"
#include "../xutil.h"

#define IS_OP_EMPTY_DIR(x) ((x)[0] == '\0')                                    // Empty path
#define IS_OP_CURR_DIR(x) ((x)[0] == '.' && (x)[1] == '\0')                    // '.'
#define IS_OP_PARENT_DIR(x) ((x)[0] == '.' && (x)[1] == '.' && (x)[2] == '\0') // '..'

namespace vfs
{
    std::vector<VfsManager *> Managers;

	std::string DriveToDos(const std::string &Drive)
	{
		// "E:\Program Files (x86)\Steam\" -> "E:"
		std::regex driveRegex("([A-z]:)");

		std::smatch matches;
		if (!std::regex_search(Drive, matches, driveRegex))
			return "";

		return util::TrimSlash("\\??\\" + matches[1].str());
	}

	std::string DriveToDevice(const std::string &Drive)
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

	std::string DriveToGuid(const std::string &Drive)
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

    bool VfsManager::ResolvePhysicalPath(std::string &VirtPath, std::string &RealPath)
    {
        VfsEntry *current = &r;

        auto moveNextDir = [&current](const char *Dir, bool Files) {
			BUG_IF(current->IsFile);

			if (IS_OP_EMPTY_DIR(Dir) || IS_OP_CURR_DIR(Dir))
				return true;

            if (IS_OP_PARENT_DIR(Dir))
            {
                if (current->Parent)
                    current = current->Parent;

                return true;
            }

            VfsEntry *e = static_cast<VfsDirEntry *>(current)->LinearFind(Dir, Files);

            if (!e)
                return false;

            current = e;
            return true;
        };

        // Now split the string based on each '\'
        char *pathPtr  = VirtPath.data();
        char *ptrStart = pathPtr;
        bool resolved  = false;

        for (;; pathPtr++)
        {
            // Traverse down to next level...
            if (*pathPtr == '\\')
            {
                *pathPtr = '\0';
                resolved = moveNextDir(ptrStart, false);
                *pathPtr = '\\';

                if (!resolved)
                    break;

                ptrStart = pathPtr + 1;
            }

            // End of string, stop parsing
            if (*pathPtr == '\0')
            {
                resolved = moveNextDir(ptrStart, true);
                break;
            }
        }

        BUG_IF(!current);

        // An alias **must** be bound
        if (!current->IsAlias)
            return false;

        // If the path was fully resolved (virtual path present)
        if (resolved)
        {
            RealPath = current->Alias;
            return true;
        }

        // Partial resolve, append the remaining phys-path to this virt-directory
        RealPath = current->Alias;
        RealPath += "\\";
        RealPath += ptrStart;
        return true;
    }

    VfsEntry *VfsManager::Resolve(std::string &VirtPath)
    {
        // Resolve directory first if possible. If there isn't one, we just
        // assume it's the root dir.
        const char *endPart = nullptr;
        auto dir            = &r;
        size_t delim        = VirtPath.find_last_of('\\');

        if (delim == std::string::npos)    // '\' not found
            endPart = VirtPath.data();     //
        else if (delim == 0)               // '\' is at root (i.e "\\MyFile.txt")
            endPart = &VirtPath.data()[1]; //
        else                               // Relative directory (i.e "Dir1\\Dir2\\Dir3\\MyFile.txt")
        {
            dir     = ResolveDir(VirtPath.substr(0, delim - 1));
            endPart = &VirtPath.data()[delim + 1];
        }

        if (!dir || endPart[0] == '\0')
            return nullptr;

        return dir->LinearFind(endPart, true);
    }

    VfsDirEntry *VfsManager::ResolveDir(std::string &VirtPath)
    {
        //
        // Split each directory into a separate traversal, i.e:
        //
        // Skyrim\Users\Administrator\..\Screenshots<end>
        //       ^     ^             ^  ^           ^
        //
        // VfsDirEntry{Screenshots} is returned.
        //
        VfsDirEntry *current = &r;

        auto moveNextDir = [&current](const char *Dir) {
			if (IS_OP_EMPTY_DIR(Dir) || IS_OP_CURR_DIR(Dir))
				return true;

            // '..' -> parent dir if there is one
            if (IS_OP_PARENT_DIR(Dir))
            {
                if (current->Parent)
                    current = current->Parent;

                return true;
            }

            current = static_cast<VfsDirEntry *>(current->LinearFind(Dir, false));

            if (!current)
                return false;

            return true;
        };

        // Now split the string based on each '\'
        char *pathPtr = VirtPath.data();

        for (char *ptrStart = pathPtr;; pathPtr++)
        {
            // Traverse down to next level...
            if (*pathPtr == '\\')
            {
                *pathPtr = '\0';
                bool ret = moveNextDir(ptrStart);
                *pathPtr = '\\';

                if (!ret)
                    return nullptr;

                ptrStart = pathPtr + 1;
            }

            // End of string, stop parsing
            if (*pathPtr == '\0')
            {
                if (!moveNextDir(ptrStart))
                    return nullptr;

                break;
            }
        }

        return current;
    }

    VfsFileEntry *VfsManager::ResolveFile(std::string &VirtPath)
    {
        VfsEntry *file = Resolve(VirtPath);

        if (!file || !file->IsFile)
            return nullptr;

        return static_cast<VfsFileEntry *>(file);
    }

    bool VfsManager::CreateFileAlias(std::string VirtPath, std::string RealPath)
    {
        util::NormalizePath(VirtPath);
        util::NormalizePath(RealPath);

        std::string rootDir;
        std::string subDir;
        util::SplitPath(VirtPath, rootDir, subDir);

        VfsDirEntry *parent = ResolveDir(rootDir);

        if (!parent)
            return false;

        // Can't create a file if name exists, even when it's a directory
        if (parent->LinearFind(subDir, false))
            return false;

        printf("REGISTER ALIAS: %s <==> %s\n", VirtPath.c_str(), RealPath.c_str());

        VfsFileEntry *dir = new VfsFileEntry;
        dir->Name         = subDir;
        dir->Parent       = parent;
        dir->IsAlias      = true;
        dir->Alias        = RealPath;
        parent->Listing.push_back(dir);
        return true;
    }

    bool VfsManager::CreateDirectory(std::string VirtPath)
    {
        //
        // Create an empty folder in memory. This folder doesn't exist anywhere on
        // a hard drive.
        //
        printf("\n------------------------\n");
        printf("INPUT: %s\n", VirtPath.c_str());

        util::NormalizePath(VirtPath);

        std::string rootDir;
        std::string subDir;
        util::SplitPath(VirtPath, rootDir, subDir);

        VfsDirEntry *parent = ResolveDir(rootDir);

        printf("RootDir: %s\n", rootDir.c_str());
        printf("SubDir: %s\n", subDir.c_str());

        if (!parent)
            return false;

        // Can't create a directory if name exists, even when it's a file
        if (parent->LinearFind(subDir, true))
        {
            printf("DIRECTORY ALREADY EXISTS (%s)\n", subDir.c_str());
            return false;
        }

        printf("Directory was valid (%s)\n", parent->Name.c_str());

        VfsDirEntry *dir = new VfsDirEntry;
        dir->Name        = subDir;
        dir->Parent      = parent;
        parent->Listing.push_back(dir);
        return true;
    }

    bool VfsManager::CreateDirectoryAlias(std::string VirtPath, std::string RealPath)
    {
        //
        // Create a folder in memory that allows pass-thru to an on-disk directory. This
        // is effectively the same as MountVirtualDirectory, but within a specific VirtPath.
        //
        util::NormalizePath(VirtPath);
        util::NormalizePath(RealPath);

        std::string rootDir;
        std::string subDir;
        util::SplitPath(VirtPath, rootDir, subDir);

        VfsDirEntry *parent = ResolveDir(rootDir);

        if (!parent)
            return false;

        // Can't create a directory if name exists, even when it's a file
        if (parent->LinearFind(subDir, true))
            return false;

        printf("REGISTER ALIAS: %s <==> %s\n", VirtPath.c_str(), RealPath.c_str());

        VfsDirEntry *dir = new VfsDirEntry;
        dir->Name        = subDir;
        dir->Parent      = parent;
        dir->IsAlias     = true;
        dir->Alias       = RealPath;
        parent->Listing.push_back(dir);
        return true;
    }

    VfsManager *VfsManager::MountVirtualDirectory(std::string VirtPath, std::string RealPath, bool AllowPassthru)
    {
        //
        // Create a new virtual filesystem manager.
        //
        using namespace util;

        NormalizePath(VirtPath);
        NormalizePath(RealPath);

        if (!PathIsValid(VirtPath) || !PathIsAbsolute(VirtPath))
            return nullptr;

        if (AllowPassthru)
        {
            if (!PathIsValid(RealPath) || !PathIsAbsolute(RealPath))
                return nullptr;

            // If the path is "real", it must be a real directory too
            DWORD pathAttributes = GetFileAttributesW(str::wide(RealPath).c_str());

            if (pathAttributes == INVALID_FILE_ATTRIBUTES)
                return nullptr;

            if (!(pathAttributes & FILE_ATTRIBUTE_DIRECTORY))
                return nullptr;
        }

        // Resolve system paths for later use
        VfsManager *sys = new VfsManager;
        sys->r.Name     = "$ROOT";
        sys->r.IsAlias  = AllowPassthru;
        sys->r.Alias    = VirtPath;

        sys->MountPoint = VirtPath;
        sys->RealPath   = RealPath;

        sys->RootDos    = DriveToDos(VirtPath);
        sys->RootDevice = DriveToDevice(VirtPath);
        sys->RootGuid   = DriveToGuid(VirtPath);

        sys->RootDosW        = str::wide(sys->RootDos);
        sys->RootDosFullW    = str::wide(sys->RootDos + sys->MountPoint.substr(2));
        sys->RootDeviceW     = str::wide(sys->RootDevice);
        sys->RootDeviceFullW = str::wide(sys->RootDevice + sys->MountPoint.substr(2));
        sys->RootGuidW       = str::wide(sys->RootGuid);
        sys->RootGuidFullW   = str::wide(sys->RootGuid + sys->MountPoint.substr(2));

        printf("%s\n", sys->MountPoint.c_str());
        printf("%s\n", sys->RealPath.c_str());
        printf("%s\n", sys->RootDos.c_str());
        printf("%s\n", sys->RootDevice.c_str());
        printf("%s\n", sys->RootGuid.c_str());

        printf("%ws\n", sys->RootDosFullW.c_str());
        printf("%ws\n", sys->RootDeviceFullW.c_str());
        printf("%ws\n", sys->RootGuidFullW.c_str());

        Managers.push_back(sys);
        printf("Added manager\n");
        return sys;
    }
}
