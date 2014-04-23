#pragma once

class OutputCallbacks : public IDebugOutputCallbacks
{
public:
	// IUnknown
	STDMETHOD(QueryInterface)(THIS_ REFIID InterfaceId, PVOID* Interface);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);

	STDMETHOD(Output)(THIS_ ULONG Mask, PCSTR Text);
};