#include "symbolsourcebase.h"
#include <algorithm>

bool SymbolSourceBase::mapSourceFilePdbToDisk(const std::string & pdb, const std::string & disk)
{
    std::string pdblower = pdb, disklower = disk;
    std::transform(pdblower.begin(), pdblower.end(), pdblower.begin(), tolower);
    std::transform(disklower.begin(), disklower.end(), disklower.begin(), tolower);

    // Abort if the disk file doesn't exist
    if(!FileExists(disklower.c_str()))
        return false;

    // Remove existing mapping if found
    auto found = _sourceFileMapPdbToDisk.find(pdblower);
    if(found != _sourceFileMapPdbToDisk.end())
    {
        // Abort if there is already an existing mapping for the destination (we want 1 to 1 mapping)
        // Done here because we can avoid corrupting the state
        if(disklower != found->second && _sourceFileMapDiskToPdb.count(disklower))
            return false;

        // Remove existing mapping
        _sourceFileMapDiskToPdb.erase(found->second);
        _sourceFileMapPdbToDisk.erase(found);
    }
    // Abort if there is already an existing mapping for the destination (we want 1 to 1 mapping)
    else if(_sourceFileMapDiskToPdb.count(disklower))
    {
        return false;
    }

    // Insert destinations
    _sourceFileMapPdbToDisk.insert({ pdblower, disk });
    _sourceFileMapDiskToPdb.insert({ disklower, pdb });
    return true;
}

bool SymbolSourceBase::getSourceFileDiskToPdb(const std::string & disk, std::string & pdb) const
{
    std::string disklower = disk;
    std::transform(disklower.begin(), disklower.end(), disklower.begin(), tolower);
    auto found = _sourceFileMapDiskToPdb.find(disklower);
    if(found == _sourceFileMapDiskToPdb.end())
        return false;
    pdb = found->second;
    return true;
}

bool SymbolSourceBase::getSourceFilePdbToDisk(const std::string & pdb, std::string & disk) const
{
    std::string pdblower = pdb;
    std::transform(pdblower.begin(), pdblower.end(), pdblower.begin(), tolower);
    auto found = _sourceFileMapPdbToDisk.find(pdblower);
    if(found == _sourceFileMapPdbToDisk.end())
        return false;
    disk = found->second;
    return true;
}