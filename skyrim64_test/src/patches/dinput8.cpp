#include "../common.h"
#include "dinput8.h"

bool ProxyIDirectInputDevice8A::m_EnableInput = true;

uint8_t *CreateDevice;
HRESULT WINAPI hk_DirectInput8CreateDevice(IDirectInput8A *thisptr, REFGUID rguid, IDirectInputDevice8A **lplpDirectInputDevice, IUnknown *pUnkOuter)
{
	HRESULT hr = ((decltype(&hk_DirectInput8CreateDevice))CreateDevice)(thisptr, rguid, lplpDirectInputDevice, pUnkOuter);

	// Return our proxy device to the caller
	if (SUCCEEDED(hr))
		*lplpDirectInputDevice = new ProxyIDirectInputDevice8A(*lplpDirectInputDevice, rguid);

	return hr;
}

HRESULT WINAPI hk_DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, IUnknown *punkOuter)
{
	HRESULT hr = DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	if (SUCCEEDED(hr))
	{
		// Only hook IDirectInput8A
		if (riidltf != IID_IDirectInput8A)
		{
			MessageBoxA(nullptr, "UNIMPLEMENTED INTERFACE", "???", 0);
			return E_FAIL;
		}

		CreateDevice = Detours::X64::DetourClassVTable(*(uint8_t **)(*ppvOut), &hk_DirectInput8CreateDevice, 3);
	}

	return hr;
}

ProxyIDirectInputDevice8A::ProxyIDirectInputDevice8A(IDirectInputDevice8A *Device, REFGUID Guid)
{
	m_Device = Device;
	m_IsMouse = (Guid == GUID_SysMouse);
	m_IsKeyboard = (Guid == GUID_SysKeyboard);

	std::unique_lock<std::shared_mutex> lock(m_Mutex);
	m_Devices.push_back(m_Device);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::QueryInterface(REFIID iid, LPVOID * ppvObject)
{
	return m_Device->QueryInterface(iid, ppvObject);
}

ULONG APIENTRY ProxyIDirectInputDevice8A::AddRef(void)
{
	return m_Device->AddRef();
}

ULONG APIENTRY ProxyIDirectInputDevice8A::Release(void)
{
	// Check if refcount would hit zero (remove from global list)
	m_Device->AddRef();

	if (m_Device->Release() == 1)
	{
		m_Mutex.lock();
		m_Devices.erase(std::remove(m_Devices.begin(), m_Devices.end(), m_Device), m_Devices.end());
		m_Mutex.unlock();

		// Grab a pointer copy and delete us
		auto temp	= m_Device;
		m_Device	= nullptr;
		delete this;

		return temp->Release();
	}

	return m_Device->Release();
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::GetCapabilities(LPDIDEVCAPS lpDIDevCaps)
{
	return m_Device->GetCapabilities(lpDIDevCaps);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACK lpCallback, LPVOID pvRef, DWORD dwFlags)
{
	return m_Device->EnumObjects(lpCallback, pvRef, dwFlags);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::GetProperty(REFGUID rguidProp, LPDIPROPHEADER pdiph)
{
	return m_Device->GetProperty(rguidProp, pdiph);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::SetProperty(REFGUID rguidProp, LPCDIPROPHEADER pdiph)
{
	return m_Device->SetProperty(rguidProp, pdiph);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::Acquire()
{
	return m_Device->Acquire();
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::Unacquire()
{
	return m_Device->Unacquire();
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::GetDeviceState(DWORD cbData, LPVOID lpvData)
{
	HRESULT result = m_Device->GetDeviceState(cbData, lpvData);

	if (SUCCEEDED(result))
	{
		// Mouse only
		if (m_IsMouse && cbData == sizeof(DIMOUSESTATE2))
		{
			ImGuiIO& io = ImGui::GetIO();
			auto ptr = (DIMOUSESTATE2 *)lpvData;

			io.MouseDown[0] = (ptr->rgbButtons[0] & 0x80) != 0;
			io.MouseDown[1] = (ptr->rgbButtons[1] & 0x80) != 0;
			io.MousePos.x += ptr->lX;
			io.MousePos.y += ptr->lY;

			io.MousePos.x = max(io.MousePos.x, 0);
			io.MousePos.y = max(io.MousePos.y, 0);
			io.MouseWheel = max(min((float)ptr->lZ, 1), -1);
		}
	}

	// if (don't forward input to game)
	if (lpvData && !m_EnableInput)
		memset(lpvData, 0, cbData);

	return result;
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::GetDeviceData(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags)
{
	HRESULT hr = m_Device->GetDeviceData(cbObjectData, rgdod, pdwInOut, dwFlags);

	// if (don't forward input to game)
	if (rgdod && !m_EnableInput)
		memset(rgdod, 0, cbObjectData * *pdwInOut);

	return hr;
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::SetDataFormat(LPCDIDATAFORMAT lpdf)
{
	return m_Device->SetDataFormat(lpdf);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::SetEventNotification(HANDLE hEvent)
{
	return m_Device->SetEventNotification(hEvent);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::SetCooperativeLevel(HWND hwnd, DWORD dwFlags)
{
	// Allow system keyboard commands (alt+tab, WINKEY, ...)
	if (m_IsKeyboard)
		dwFlags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

	return m_Device->SetCooperativeLevel(hwnd, dwFlags);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::GetObjectInfo(LPDIDEVICEOBJECTINSTANCE pdidoi, DWORD dwObj, DWORD dwHow)
{
	return m_Device->GetObjectInfo(pdidoi, dwObj, dwHow);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::GetDeviceInfo(LPDIDEVICEINSTANCE pdidi)
{
	return m_Device->GetDeviceInfo(pdidi);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::RunControlPanel(HWND hwndOwner, DWORD dwFlags)
{
	return m_Device->RunControlPanel(hwndOwner, dwFlags);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::Initialize(HINSTANCE hinst, DWORD dwVersion, REFGUID rguid)
{
	return m_Device->Initialize(hinst, dwVersion, rguid);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::CreateEffect(REFGUID rguid, LPCDIEFFECT lpeff, LPDIRECTINPUTEFFECT *ppdeff, LPUNKNOWN punkOuter)
{
	return m_Device->CreateEffect(rguid, lpeff, ppdeff, punkOuter);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::EnumEffects(LPDIENUMEFFECTSCALLBACK lpCallback, LPVOID pvRef, DWORD dwEffType)
{
	return m_Device->EnumEffects(lpCallback, pvRef, dwEffType);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::GetEffectInfo(LPDIEFFECTINFO pdei, REFGUID rguid)
{
	return m_Device->GetEffectInfo(pdei, rguid);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::GetForceFeedbackState(LPDWORD pdwOut)
{
	return m_Device->GetForceFeedbackState(pdwOut);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::SendForceFeedbackCommand(DWORD dwFlags)
{
	return m_Device->SendForceFeedbackCommand(dwFlags);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback, LPVOID pvRef, DWORD fl)
{
	return m_Device->EnumCreatedEffectObjects(lpCallback, pvRef, fl);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::Escape(LPDIEFFESCAPE pesc)
{
	return m_Device->Escape(pesc);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::Poll()
{
	return m_Device->Poll();
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::SendDeviceData(DWORD cbObjectData, LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD fl)
{
	return m_Device->SendDeviceData(cbObjectData, rgdod, pdwInOut, fl);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::EnumEffectsInFile(LPCSTR lpszFileName, LPDIENUMEFFECTSINFILECALLBACK pec, LPVOID pvRef, DWORD dwFlags)
{
	return m_Device->EnumEffectsInFile(lpszFileName, pec, pvRef, dwFlags);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::WriteEffectToFile(LPCSTR lpszFileName, DWORD dwEntries, LPDIFILEEFFECT rgDiFileEft, DWORD dwFlags)
{
	return m_Device->WriteEffectToFile(lpszFileName, dwEntries, rgDiFileEft, dwFlags);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::BuildActionMap(LPDIACTIONFORMAT lpdiaf, LPCTSTR lpszUserName, DWORD dwFlags)
{
	return m_Device->BuildActionMap(lpdiaf, lpszUserName, dwFlags);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::SetActionMap(LPDIACTIONFORMAT lpdiActionFormat, LPCTSTR lptszUserName, DWORD dwFlags)
{
	return m_Device->SetActionMap(lpdiActionFormat, lptszUserName, dwFlags);
}

HRESULT APIENTRY ProxyIDirectInputDevice8A::GetImageInfo(LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader)
{
	return m_Device->GetImageInfo(lpdiDevImageInfoHeader);
}

void ProxyIDirectInputDevice8A::ToggleGlobalInput(bool EnableInput)
{
	m_EnableInput = EnableInput;
}

bool ProxyIDirectInputDevice8A::GlobalInputAllowed()
{
	return m_EnableInput;
}