#include "symbolsourcepdb.h"

SymbolSourcePDB::~SymbolSourcePDB()
{
	if (_pdb.isOpen())
	{
		_pdb.close();
	}
}

bool SymbolSourcePDB::loadPDB(const std::string& path, duint imageBase)
{
	if (!PDBDiaFile::initLibrary())
	{
		return false;
	}
	return _pdb.open(path.c_str());
}

bool SymbolSourcePDB::isLoaded() const
{
	return _pdb.isOpen();
}

bool SymbolSourcePDB::findSymbolExact(duint rva, SymbolInfo& symInfo)
{
	if (SymbolSourceBase::isAddressInvalid(rva))
		return false;

	auto it = _symbols.find(rva);
	if (it != _symbols.end())
	{
		symInfo = (*it).second;
		return true;
	}

	DiaSymbol_t sym;
	if (_pdb.findSymbolRVA(rva, sym) && sym.disp == 0)
	{
		symInfo.addr = rva;
		symInfo.disp = 0;
		symInfo.size = sym.size;
		symInfo.decoratedName = sym.undecoratedName;
		symInfo.undecoratedName = sym.undecoratedName;
		symInfo.valid = true;

		_symbols.insert(rva, symInfo);

		return true;
	}

	markAdressInvalid(rva);
	return false;
}

template <typename A, typename B>
typename A::iterator findExactOrLower(A& ctr, const B key)
{
	if (ctr.empty())
		return ctr.end();

	auto itr = ctr.lower_bound(key);

	if (itr == ctr.begin() && (*itr).first != key)
		return ctr.end();
	else if (itr == ctr.end() || (*itr).first != key)
		return --itr;

	return itr;
}

bool SymbolSourcePDB::findSymbolExactOrLower(duint rva, SymbolInfo& symInfo)
{
	auto it = findExactOrLower(_symbols, rva);
	if (it != _symbols.end())
	{
		symInfo = (*it).second;
		symInfo.disp = (int32_t)(rva - symInfo.addr);
		return true;
	}

	DiaSymbol_t sym;
	if (_pdb.findSymbolRVA(rva, sym))
	{
		symInfo.addr = rva;
		symInfo.disp = sym.disp;
		symInfo.size = sym.size;
		symInfo.decoratedName = sym.undecoratedName;
		symInfo.undecoratedName = sym.undecoratedName;
		symInfo.valid = true;

		return true;
	}

	return nullptr;
}

