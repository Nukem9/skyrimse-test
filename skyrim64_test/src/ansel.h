#pragma once

namespace Ansel
{
#pragma pack(push, 1)
	struct ConfigData
	{
		float forward[3];
		float up[3];
		float right[3];

		// The speed at which camera moves in the world
		float translationalSpeedInWorldUnitsPerSecond;
		// The speed at which camera rotates 
		float rotationalSpeedInDegreesPerSecond;
		// How many frames it takes for camera update to be reflected in a rendered frame
		unsigned int captureLatency;
		// How many frames we must wait for a new frame to settle - i.e. temporal AA and similar
		// effects to stabilize after the camera has been adjusted
		unsigned int captureSettleLatency;
		// Game scale, the size of a world unit measured in meters
		float metersInWorldUnit;
		// Integration will support Camera::screenOriginXOffset/screenOriginYOffset
		bool isCameraOffcenteredProjectionSupported;
		// Integration will support Camera::position
		bool isCameraTranslationSupported;
		// Integration will support Camera::rotation
		bool isCameraRotationSupported;
		// Integration will support Camera::horizontalFov
		bool isCameraFovSupported;
		// Integration allows a filter/effect to remain active when the Ansel session is not active
		bool isFilterOutsideSessionAllowed;
	};

	struct CameraData
	{
		float fov; // Degrees
		float projectionOffset[2];
		float position[3];
		float rotation[4];
	};

	struct SessionData
	{
		// If set to false none of the below parameters are relevant
		bool isAnselAllowed;
		bool is360MonoAllowed;
		bool is360StereoAllowed;
		bool isFovChangeAllowed;
		bool isHighresAllowed;
		bool isPauseAllowed;
		bool isRotationAllowed;
		bool isTranslationAllowed;
	};
#pragma pack(pop)
}

void anselInit(Ansel::ConfigData *conf);			// Start
void anselConfigureSession(Ansel::SessionData *ses);// Start
void anselUpdateCamera(Ansel::CameraData *cam);		// OnPreRender
bool anselIsSessionOn();
bool anselIsCaptureOn();
bool anselIsAvailable();