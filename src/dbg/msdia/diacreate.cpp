#include <windows.h>
#include "diacreate.h"

typedef HRESULT(__stdcall* pDllGetClassObject)(
    _In_  REFCLSID rclsid,
    _In_  REFIID   riid,
    _Out_ LPVOID*   ppv
);


HRESULT STDMETHODCALLTYPE NoRegCoCreate(const __wchar_t* dllName,
                                        REFCLSID   rclsid,
                                        REFIID     riid,
                                        void**     ppv)
{
    HRESULT hr;
    HMODULE hModule = LoadLibraryW(dllName);
    pDllGetClassObject DllGetClassObject;
    if(hModule && (DllGetClassObject = (pDllGetClassObject)GetProcAddress(hModule, "DllGetClassObject")))
    {
        IClassFactory* classFactory;
        hr = DllGetClassObject(rclsid, IID_IClassFactory, (LPVOID*)&classFactory);
        if(SUCCEEDED(hr))
        {
            hr = classFactory->CreateInstance(nullptr, riid, ppv);
            classFactory->AddRef();
        }
    }
    else
    {
        hr = GetLastError();
        if(hr > 0)
            hr |= REASON_LEGACY_API;
    }
    return hr;
}