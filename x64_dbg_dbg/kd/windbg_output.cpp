#include "../_global.h"
#include "../console.h"
#include "stdafx.h"

// IUnknown
STDMETHODIMP OutputCallbacks::QueryInterface(THIS_ IN REFIID InterfaceId, OUT PVOID *Interface)
{
#if _MSC_VER >= 1100
	if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
		IsEqualIID(InterfaceId, __uuidof(IDebugOutputCallbacks)))
#else
	if (IsEqualIID(InterfaceId, IID_IUnknown) ||
		IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks))
#endif
	{
		*Interface = (IDebugEventCallbacks *)this;

		this->AddRef();
		return S_OK;
	}

	*Interface = NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) OutputCallbacks::AddRef(THIS)
{
	return 1;
}

STDMETHODIMP_(ULONG) OutputCallbacks::Release(THIS)
{
	return 0;
}

char *Filter[] =
{
	"SXS.D", // SXS.DLL messages
	"PopPe", // PopPep messages
	"PdcpA", // PdcpActivatorReceive
	"PdcRe", // PdcResiliencyClientReference
	"Resil", // Resiliency client
};

// IDebugOutputCallbacks
STDMETHODIMP OutputCallbacks::Output(THIS_ IN ULONG Mask, IN PCSTR Text)
{
	UNREFERENCED_PARAMETER(Mask);

	// Don't care if the line starts with *
	if (Text[0] == '*')
		return S_OK;

	// Filters
	for (int i = 0; i < ARRAYSIZE(Filter); i++)
	{
		if (!memcmp(Text, Filter[i], 5))
			return S_OK;
	}

	dprintf(Text);

	return S_OK;
}