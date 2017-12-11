// diacreate.h - creation helper functions for DIA initialization
//-----------------------------------------------------------------
//
// Copyright Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------
#ifndef _DIACREATE_H_
#define _DIACREATE_H_

//
// Create a dia data source object from the dia dll (by dll name - does not access the registry).
//

HRESULT STDMETHODCALLTYPE NoRegCoCreate(const __wchar_t* dllName,
                                        REFCLSID   rclsid,
                                        REFIID     riid,
                                        void**     ppv);

#ifndef _NATIVE_WCHAR_T_DEFINED
#ifdef __cplusplus

HRESULT STDMETHODCALLTYPE NoRegCoCreate(const wchar_t* dllName,
                                        REFCLSID   rclsid,
                                        REFIID     riid,
                                        void**     ppv)
{
    return NoRegCoCreate((const __wchar_t*)dllName, rclsid, riid, ppv);
}

#endif
#endif



//
// Create a dia data source object from the dia dll (looks up the class id in the registry).
//
HRESULT STDMETHODCALLTYPE NoOleCoCreate(REFCLSID   rclsid,
                                        REFIID     riid,
                                        void**     ppv);

#endif
