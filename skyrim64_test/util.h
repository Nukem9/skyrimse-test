#pragma once

intptr_t FindPattern(const std::vector<unsigned char>& data, intptr_t baseAddress, const unsigned char *lpPattern, const char *pszMask, intptr_t offset, intptr_t resultUsage);
void PatchMemory(ULONG_PTR Address, PBYTE Data, SIZE_T Size);