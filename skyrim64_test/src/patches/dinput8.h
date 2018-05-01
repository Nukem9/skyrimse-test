#pragma once

#define DIRECTINPUT_VERSION 0x800
#include <dinput.h>
#include <shared_mutex>
#include <vector>

interface ProxyIDirectInputDevice8A : public IDirectInputDevice8A
{
private:
	IDirectInputDevice8A *m_Device;
	bool m_IsKeyboard;
	bool m_IsMouse;

	inline static std::shared_mutex m_Mutex;
	inline static std::vector<IDirectInputDevice8A *> m_Devices;
	static bool m_EnableInput;

public:
	ProxyIDirectInputDevice8A(IDirectInputDevice8A *Device, REFGUID Guid);

	HRESULT APIENTRY QueryInterface(REFIID iid, LPVOID * ppvObject) override;
	ULONG APIENTRY AddRef(void) override;
	ULONG APIENTRY Release(void) override;

	HRESULT APIENTRY GetCapabilities(LPDIDEVCAPS lpDIDevCaps) override;
	HRESULT APIENTRY EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACK lpCallback, LPVOID pvRef, DWORD dwFlags) override;
	HRESULT APIENTRY GetProperty(REFGUID rguidProp, LPDIPROPHEADER pdiph) override;
	HRESULT APIENTRY SetProperty(REFGUID rguidProp, LPCDIPROPHEADER pdiph) override;
	HRESULT APIENTRY Acquire() override;
	HRESULT APIENTRY Unacquire() override;
	HRESULT APIENTRY GetDeviceState(DWORD cbData, LPVOID lpvData) override;
	HRESULT APIENTRY GetDeviceData(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) override;
	HRESULT APIENTRY SetDataFormat(LPCDIDATAFORMAT lpdf) override;
	HRESULT APIENTRY SetEventNotification(HANDLE hEvent) override;
	HRESULT APIENTRY SetCooperativeLevel(HWND hwnd, DWORD dwFlags) override;
	HRESULT APIENTRY GetObjectInfo(LPDIDEVICEOBJECTINSTANCE pdidoi, DWORD dwObj, DWORD dwHow) override;
	HRESULT APIENTRY GetDeviceInfo(LPDIDEVICEINSTANCE pdidi) override;
	HRESULT APIENTRY RunControlPanel(HWND hwndOwner, DWORD dwFlags) override;
	HRESULT APIENTRY Initialize(HINSTANCE hinst, DWORD dwVersion, REFGUID rguid) override;
	HRESULT APIENTRY CreateEffect(REFGUID rguid, LPCDIEFFECT lpeff, LPDIRECTINPUTEFFECT *ppdeff, LPUNKNOWN punkOuter) override;
	HRESULT APIENTRY EnumEffects(LPDIENUMEFFECTSCALLBACK lpCallback, LPVOID pvRef, DWORD dwEffType) override;
	HRESULT APIENTRY GetEffectInfo(LPDIEFFECTINFO pdei, REFGUID rguid) override;
	HRESULT APIENTRY GetForceFeedbackState(LPDWORD pdwOut) override;
	HRESULT APIENTRY SendForceFeedbackCommand(DWORD dwFlags) override;
	HRESULT APIENTRY EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback, LPVOID pvRef, DWORD fl) override;
	HRESULT APIENTRY Escape(LPDIEFFESCAPE pesc) override;
	HRESULT APIENTRY Poll() override;
	HRESULT APIENTRY SendDeviceData(DWORD cbObjectData, LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD fl) override;
	HRESULT APIENTRY EnumEffectsInFile(LPCSTR lpszFileName, LPDIENUMEFFECTSINFILECALLBACK pec, LPVOID pvRef, DWORD dwFlags) override;
	HRESULT APIENTRY WriteEffectToFile(LPCSTR lpszFileName, DWORD dwEntries, LPDIFILEEFFECT rgDiFileEft, DWORD dwFlags) override;
	HRESULT APIENTRY BuildActionMap(LPDIACTIONFORMAT lpdiaf, LPCTSTR lpszUserName, DWORD dwFlags) override;
	HRESULT APIENTRY SetActionMap(LPDIACTIONFORMAT lpdiActionFormat, LPCTSTR lptszUserName, DWORD dwFlags) override;
	HRESULT APIENTRY GetImageInfo(LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader) override;

	static void ToggleGlobalInput(bool EnableInput);
	static bool GlobalInputAllowed();
};

HRESULT WINAPI hk_DirectInput8CreateDevice(IDirectInput8A *thisptr, REFGUID rguid, IDirectInputDevice8A **lplpDirectInputDevice, IUnknown *pUnkOuter);
HRESULT WINAPI hk_DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, IUnknown *punkOuter);