#include "mmNotificationClient.h"

//ランダムなタイミングで飛んでくるのでフラグ上げ下げに置き換えて適切なタイミングでcoreからポーリングさせる
//関数ポインタなど使ってこのクラスから直接defaultDevice置き換えなどさせると危険
//(旧デバイスで音量取得→デバイス切替→音量操作　の時にまだ新デバイス取得できておらず みたいなことが起こりえる)

//コンストラクタ
mmNotificationClient::mmNotificationClient()
{
	m_isDefaultDeviceChanged = false;
    _cRef = 1;
}

HRESULT STDMETHODCALLTYPE mmNotificationClient::OnDeviceStateChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) {
	//何もしない
	return S_OK;
};

HRESULT STDMETHODCALLTYPE mmNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId) {
	//何もしない
	return S_OK;
};

HRESULT STDMETHODCALLTYPE mmNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId) {
	//何もしない
	return S_OK;
};

HRESULT STDMETHODCALLTYPE mmNotificationClient::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) {
	//何もしない
	return S_OK;
};

HRESULT STDMETHODCALLTYPE mmNotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) {
	//出力デバイス変化通知が来たらフラグセット
    if (flow == eRender && role == TARGET_ROLE) {
		m_isDefaultDeviceChanged = true;
	}
	return S_OK;
};

HRESULT STDMETHODCALLTYPE mmNotificationClient::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) {
	//何もしない
	return S_OK;
};

void mmNotificationClient::clear_isDefaultDeviceChanged(void) {
	m_isDefaultDeviceChanged = false;
}

bool mmNotificationClient::get_isDefaultDeviceChanged(void) {
	return m_isDefaultDeviceChanged;
}

// IUnknownの必須メソッド
// OSSライセンス対象外 Not covered by OSS license
// https://learn.microsoft.com/ja-jp/windows/win32/coreaudio/device-events
// 引用開始 start quote
// IUnknown methods -- AddRef, Release, and QueryInterface

ULONG STDMETHODCALLTYPE mmNotificationClient::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG STDMETHODCALLTYPE mmNotificationClient::Release()
{
    ULONG ulRef = InterlockedDecrement(&_cRef);
    if (0 == ulRef)
    {
        delete this;
    }
    return ulRef;
}

HRESULT STDMETHODCALLTYPE mmNotificationClient::QueryInterface(REFIID riid, VOID** ppvInterface)
{
    if (IID_IUnknown == riid)
    {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    }
    else if (__uuidof(IMMNotificationClient) == riid)
    {
        AddRef();
        *ppvInterface = (IMMNotificationClient*)this;
    }
    else
    {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}
//引用終了 end quote