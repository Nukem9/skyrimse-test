#include "stdafx.h"

HMODULE g_AnselModule;
void(*g_AnselInit)(Ansel::ConfigData *conf);
void(*g_AnselConfigureSession)(Ansel::SessionData *ses);
void(*g_AnselUpdateCamera)(Ansel::CameraData *cam);
bool(*g_AnselIsSessionOn)();
bool(*g_AnselIsCaptureOn)();
bool(*g_AnselIsAvailable)();

void anselInit(Ansel::ConfigData *conf)
{
	g_AnselModule = GetModuleHandleA("AnselPlugin64.dll");

	if (!g_AnselModule)
		g_AnselModule = LoadLibraryA("AnselPlugin64.dll");

	if (!g_AnselModule)
	{
		MessageBoxA(nullptr, "FAILED TO GET ANSEL DLL", "", 0);
		return;
	}

	// Resolve dynamic imports
	*(FARPROC *)&g_AnselInit = GetProcAddress(g_AnselModule, "anselInit");
	*(FARPROC *)&g_AnselConfigureSession = GetProcAddress(g_AnselModule, "anselConfigureSession");
	*(FARPROC *)&g_AnselUpdateCamera = GetProcAddress(g_AnselModule, "anselUpdateCamera");
	*(FARPROC *)&g_AnselIsSessionOn = GetProcAddress(g_AnselModule, "anselIsSessionOn");
	*(FARPROC *)&g_AnselIsCaptureOn = GetProcAddress(g_AnselModule, "anselIsCaptureOn");
	*(FARPROC *)&g_AnselIsAvailable = GetProcAddress(g_AnselModule, "anselIsAvailable");

	// Initialize
	if (g_AnselInit)
		g_AnselInit(conf);
}

void anselConfigureSession(Ansel::SessionData *ses)
{
	if (g_AnselConfigureSession)
		g_AnselConfigureSession(ses);
}

void anselUpdateCamera(Ansel::CameraData *cam)
{
	if (g_AnselUpdateCamera)
		g_AnselUpdateCamera(cam);
}

bool anselIsSessionOn()
{
	if (g_AnselIsSessionOn)
		return g_AnselIsSessionOn();

	return false;
}

bool anselIsCaptureOn()
{
	if (g_AnselIsCaptureOn)
		return g_AnselIsCaptureOn();

	return false;
}

bool anselIsAvailable()
{
	if (g_AnselIsAvailable)
		return g_AnselIsAvailable();

	return true;
}

bool AnselInit = false;

bool sessionActive = false;
bool captureActive = false;

void AnselTest()
{
	if (!AnselInit)
	{
		AnselInit = true;

		Ansel::ConfigData config;
		memset(&config, 0, sizeof(config));

		config.right[0] = 1.0f;
		config.right[1] = 0.0f;
		config.right[2] = 0.0f;

		config.up[0] = 0.0f;
		config.up[1] = 1.0f;
		config.up[2] = 0.0f;

		config.forward[0] = 0.0f;
		config.forward[1] = 0.0f;
		config.forward[2] = 1.0f;

		config.translationalSpeedInWorldUnitsPerSecond = 1.0f;
		config.rotationalSpeedInDegreesPerSecond = 1.0f;
		config.captureLatency = 1;
		config.captureSettleLatency = 1;
		config.metersInWorldUnit = 12.0f;

		config.isCameraOffcenteredProjectionSupported = true;
		config.isCameraRotationSupported = true;
		config.isCameraTranslationSupported = true;
		config.isCameraFovSupported = true;
		config.isFilterOutsideSessionAllowed = true;
		anselInit(&config);

		if (anselIsAvailable())
		{
			Ansel::SessionData session;
			memset(&session, 0, sizeof(session));

			session.isAnselAllowed = true;
			session.isFovChangeAllowed = false;
			session.isHighresAllowed = true;
			session.isPauseAllowed = true;
			session.isRotationAllowed = true;
			session.isTranslationAllowed = true;
			session.is360StereoAllowed = true;
			session.is360MonoAllowed = true;
			anselConfigureSession(&session);
		}
	}

	if (anselIsSessionOn())
	{
		// Ansel session is active (user pressed Alt+F2)
		if (!sessionActive)
		{
			sessionActive = true;
			MessageBoxA(nullptr, "Started Ansel session", "", 0);
		}

		captureActive = anselIsCaptureOn();

		Ansel::CameraData cam;
		memset(&cam, 0, sizeof(cam));

		cam.fov = 90.0f;
		cam.projectionOffset[0] = 0.0f;
		cam.projectionOffset[1] = 0.0f;

		cam.position[0] = 100.0f;
		cam.position[1] = 100.0f;
		cam.position[2] = 100.0f;

		cam.rotation[0] = 0.0f;
		cam.rotation[1] = 0.0f;
		cam.rotation[2] = 0.0f;
		cam.rotation[3] = 0.0f;
		anselUpdateCamera(&cam);
	}
	else
	{
		// Ansel session is no longer active
		if (sessionActive)
		{
			sessionActive = false;
			captureActive = false;
		}
	}
}