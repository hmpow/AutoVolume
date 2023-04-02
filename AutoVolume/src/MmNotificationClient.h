#pragma once
#ifndef MMNOTIFICATIONCLIENT
#define MMNOTIFICATIONCLIENT

#include <mmdeviceapi.h>
#include "AutoVolumeSetting.h"

class mmNotificationClient : public IMMNotificationClient
{
public:
    mmNotificationClient();
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState);
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);
    void clear_isDefaultDeviceChanged(void);
    bool get_isDefaultDeviceChanged(void);

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface);
private:
    bool m_isDefaultDeviceChanged;
    LONG _cRef;
};

#endif