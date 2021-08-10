#include <windows.h>
#include <stdio.h>
#include "Loader.h"
#include "CreationKit.h"
#include "LipSynchAnim.h"

uint32_t g_CreationKitPID;
HANDLE g_NotifyEvent;
HANDLE g_WaitEvent;

DWORD WINAPI ExitNotifyThread(LPVOID Arg)
{
	// Grab handle to parent process (check for termination)
	HANDLE parentProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, g_CreationKitPID);

	if (!parentProcess)
	{
		printf("A CreationKit parent process ID (0x%X) was supplied, but wasn't able to be queried (%d).\n", g_CreationKitPID, GetLastError());
		return 1;
	}

	for (DWORD exitCode;; Sleep(2000))
	{
		if (GetExitCodeProcess(parentProcess, &exitCode) && exitCode == STILL_ACTIVE)
			continue;

		g_CreationKitPID = 0;

		CloseHandle(parentProcess);
		SetEvent(g_NotifyEvent);
		break;
	}

	return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

#if 0
	/*
	CUZJTEUgKmY7DQoNCglpZiAoZm9wZW5fcygmZiwgIkNyZWF0aW9uS2l0LnVucGF
	ja2VkLmV4ZSIsICJyYiIpID09IDApDQoJew0KCQlmc2VlayhmLCAwLCBTRUVLX0
	VORCk7DQoJCWxvbmcgc2l6ZSA9IGZ0ZWxsKGYpOw0KCQlyZXdpbmQoZik7DQoNC
	gkJY2hhciAqZXhlRGF0YSA9IG5ldyBjaGFyW3NpemVdOw0KCQlmcmVhZChleGVE
	YXRhLCBzaXplb2YoY2hhciksIHNpemUsIGYpOw0KCQlzdGJfY29tcHJlc3NfdG9
	maWxlKChjaGFyICopImNvbXByZXNzZWRfcnNyYy5iaW4iLCBleGVEYXRhLCBzaX
	plKTsNCgkJZmNsb3NlKGYpOw0KCQlkZWxldGVbXSBleGVEYXRhOw0KCX0NCg0KC
	XJldHVybiAwOw==
	*/
#endif

	// Bail immediately if this wasn't launched from the CK
	if (!getenv("Ckpid") || strlen(getenv("Ckpid")) <= 0)
		return 1;

	g_CreationKitPID = atoi(getenv("Ckpid"));

	// Establish tunnel
	char temp[128];
	sprintf_s(temp, "CkSharedMem%d", g_CreationKitPID);

	HANDLE mapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, TRUE, temp);

	if (!mapping)
	{
		printf("Could not create file mapping object (%d).\n", GetLastError());
		return 1;
	}

	auto tunnel = reinterpret_cast<CreationKit::LipGenTunnel *>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0x40000));

	if (!tunnel)
	{
		printf("Could not map view of file (%d).\n", GetLastError());
		CloseHandle(mapping);
		return 1;
	}

	sprintf_s(temp, "CkNotifyEvent%d", g_CreationKitPID);
	g_NotifyEvent = OpenEventA(EVENT_ALL_ACCESS, TRUE, temp);

	sprintf_s(temp, "CkWaitEvent%d", g_CreationKitPID);
	g_WaitEvent = OpenEventA(EVENT_ALL_ACCESS, TRUE, temp);

	if (!g_NotifyEvent || !g_WaitEvent)
	{
		printf("Could not open event handle(s) (%d).\n", GetLastError());
		UnmapViewOfFile(tunnel);
		CloseHandle(mapping);
		return 1;
	}

	CloseHandle(CreateThread(nullptr, 0, ExitNotifyThread, nullptr, 0, nullptr));

	//
	// Wait until the creation kit asks to do something
	//
	for (bool ckCodeLoaded = false;;)
	{
		DWORD waitStatus = WaitForSingleObject(g_NotifyEvent, 5000);

		if (waitStatus == WAIT_FAILED)
		{
			printf("Failed waiting for CKIT64 notification (%d).\n", GetLastError());
			break;
		}

		// If the parent process exited, bail
		if ((waitStatus == WAIT_OBJECT_0 && strlen(tunnel->InputWAVPath) <= 0) || g_CreationKitPID == 0)
			break;

		if (waitStatus == WAIT_TIMEOUT)
			continue;

		if (!ckCodeLoaded)
		{
			ckCodeLoaded = true;

			if (!Loader::Initialize())
				return 1;
		}

		printf("Attempting to create LIP file:\n");
		printf("     Language: '%s'\n", tunnel->Language);
		printf("     Input data: '%s'\n", tunnel->FonixDataPath);
		printf("     Input WAV: '%s'\n", tunnel->InputWAVPath);
		printf("     Resampled input WAV: '%s'\n", tunnel->ResampleTempWAVPath);
		printf("     Text: '%s'\n", tunnel->DialogueText);

		CreationKit::SetFaceFXDataPath(tunnel->FonixDataPath);
		CreationKit::SetFaceFXLanguage(tunnel->Language);

		// Generate the lip file, then tell the CK
		auto lipAnim = LipSynchAnim::Generate(tunnel->InputWAVPath, tunnel->ResampleTempWAVPath, tunnel->DialogueText, nullptr);

		// Write it to disk as a temp copy & free memory
		if (lipAnim)
		{
			printf("Writing temporary LIP file to 'Data\\Sound\\Voice\\Processing\\Temp.lip'\n");

			if (!lipAnim->SaveToFile("Data\\Sound\\Voice\\Processing\\Temp.lip", true, 16, true))
				printf("LIP data was generated but the file couldn't be saved!\n");

			lipAnim->Free();
		}

		memset(tunnel->InputWAVPath, 0, sizeof(tunnel->InputWAVPath));
		tunnel->UnknownStatus = true;

		// Done
		SetEvent(g_WaitEvent);
		WaitForSingleObject(g_NotifyEvent, INFINITE);
		SetEvent(g_WaitEvent);
	}

	CloseHandle(g_NotifyEvent);
	CloseHandle(g_WaitEvent);
	UnmapViewOfFile(tunnel);
	CloseHandle(mapping);

	printf("LIPGen tool exiting\n");
	return 0;
}