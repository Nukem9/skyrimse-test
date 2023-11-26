#include <stdio.h>
#include <malloc.h>

#include "profiler_internal.h"

uint32_t CRC32_file(const char* filename)
{
	FILE* Stream;
	if (!fopen_s(&Stream, filename, "rb"))
	{
		fseek(Stream, 0, SEEK_END);
		uint32_t Size = ftell(Stream);
		if (!Size)
		{
			fclose(Stream);
			return 0xFFFFFFFF;
		}

		fseek(Stream, 0, SEEK_SET);

		auto Buffer = (unsigned char*)malloc(Size + 1);
		if (!Buffer)
		{
			fclose(Stream);
			return 0xFFFFFFFF;
		}

		memset(Buffer, 0, Size + 1);
		fread(Buffer, 1, Size, Stream);
		fclose(Stream);

		uint32_t crc = 0xFFFFFFFF;

		for (uint32_t i = 0; i < Size; ++i)
			crc = crc_table[(crc ^ Buffer[i]) & 0xFF] ^ (crc >> 8);

		free(Buffer);

		return ~crc;
	}

	return 0xFFFFFFFF;
}