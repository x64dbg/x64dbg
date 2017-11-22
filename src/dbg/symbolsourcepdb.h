#ifndef _SYMBOLSOURCEPDB_H_
#define _SYMBOLSOURCEPDB_H_

#include "pdbdiafile.h"
#include "symbolsourcebase.h"

class SymbolSourcePDB : public SymbolSourceBase
{
private:
	PDBDiaFile _pdb;
	std::map<duint, SymbolInfo> _symbols;

public:
	static bool isLibraryAvailable()
	{
		return PDBDiaFile::initLibrary();
	}

public:
	virtual ~SymbolSourcePDB();

	virtual bool isLoaded() const;

	virtual bool findSymbolExact(duint rva, SymbolInfo& symInfo);

	virtual bool findSymbolExactOrLower(duint rva, SymbolInfo& symInfo);

public:
	bool loadPDB(const std::string& path, duint imageBase);
};

#endif // _SYMBOLSOURCEPDB_H_
