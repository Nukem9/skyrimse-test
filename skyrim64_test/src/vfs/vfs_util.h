#pragma once

#include <string>

namespace vfs::str
{
	std::wstring wide(const std::string &s);
	std::string narrow(const wchar_t *s);
}

namespace vfs::util
{
	bool PathIsValid(const std::string &Path);
	bool PathIsAbsolute(const std::string &Path);
	void NormalizePath(std::string &Path);
	void SplitPath(const std::string &In, std::string &Path, std::string &Part);
	std::string &TrimSlash(std::string &Path);
}