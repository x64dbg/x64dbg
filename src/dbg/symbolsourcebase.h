#ifndef _SYMBOLSOURCEBASE_H_
#define _SYMBOLSOURCEBASE_H_

#include "_global.h"

#include <stdint.h>
#include <vector>

struct SymbolInfo
{
	duint addr;
	duint size;
	int32 disp;
	String decoratedName;
	String undecoratedName;
	bool valid;
};

class SymbolSourceBase
{
private:
	std::vector<uint8_t> _symbolBitmap;

public:
	virtual ~SymbolSourceBase() = default;

	void resizeSymbolBitmap(size_t imageSize)
	{
		if (!isLoaded())
			return;

		_symbolBitmap.resize(imageSize);
		std::fill(_symbolBitmap.begin(), _symbolBitmap.end(), false);
	}

	void markAdressInvalid(uint32_t rva)
	{
		if (_symbolBitmap.empty())
			return;

		_symbolBitmap[rva] = true;
	}

	bool isAddressInvalid(uint32_t rva) const
	{
		if (_symbolBitmap.empty())
			return false;

		return !!_symbolBitmap[rva];
	}

	// Tells us if the symbols are loaded for this module.
	virtual bool isLoaded() const
	{
		return false; // Stub
	}

	// Get the symbol at the specified address, will return false if not found.
	virtual bool findSymbolExact(duint rva, SymbolInfo& symInfo)
	{
		return false; // Stub
	}

	// Get the symbol at the address or the closest behind, in case it got the closest it will set disp to non-zero, false on nothing.
	virtual bool findSymbolExactOrLower(duint rva, SymbolInfo& symInfo)
	{
		return false; // Stub
	}
};

static SymbolSourceBase EmptySymbolSource;

#endif // _SYMBOLSOURCEBASE_H_