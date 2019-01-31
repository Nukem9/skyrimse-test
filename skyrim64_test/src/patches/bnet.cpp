#include "../common.h"

size_t BNetConvertUnicodeString(char *Destination, size_t DestSize, const wchar_t *Source, size_t SourceSize)
{
	if (Destination)
		memset(Destination, 0, DestSize);

	size_t outLen;
	errno_t result = wcsrtombs_s(&outLen, Destination, DestSize, &Source, SourceSize, nullptr);

	if (result != 0)
	{
		// Try a fall back instead of normally returning -1
		outLen = WideCharToMultiByte(CP_UTF8, 0, Source, SourceSize, Destination, DestSize, nullptr, nullptr);

		if (Destination && DestSize > 0)
			Destination[DestSize - 1] = '\0';

		AssertMsg(outLen != 0, "Unicode conversion failed");
	}

	return outLen - 1;
}