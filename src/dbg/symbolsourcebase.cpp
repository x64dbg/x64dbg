#include "symbolsourcebase.h"
#include <algorithm>

bool NameIndex::findByPrefix(const std::vector<NameIndex> & byName, const std::string & prefix, const std::function<bool(const NameIndex &)> & cbFound, bool caseSensitive)
{
    struct PrefixCmp
    {
        PrefixCmp(size_t n) : n(n) { }

        bool operator()(const NameIndex & a, const NameIndex & b)
        {
            return cmp(a, b, false) < 0;
        }

        int cmp(const NameIndex & a, const NameIndex & b, bool caseSensitive)
        {
            return (caseSensitive ? strncmp : _strnicmp)(a.name, b.name, n);
        }

    private:
        size_t n;
    } prefixCmp(prefix.size());

    if(byName.empty())
        return false;

    NameIndex find;
    find.name = prefix.c_str();
    find.index = -1;
    auto found = binary_find(byName.begin(), byName.end(), find, prefixCmp);
    if(found == byName.end())
        return false;

    bool result = false;
    for(; found != byName.end() && prefixCmp.cmp(find, *found, false) == 0; ++found)
    {
        if(!caseSensitive || prefixCmp.cmp(find, *found, true) == 0)
        {
            result = true;
            if(!cbFound(*found))
                break;
        }
    }

    return result;
}

bool NameIndex::findByName(const std::vector<NameIndex> & byName, const std::string & name, NameIndex & foundIndex, bool caseSensitive)
{
    NameIndex find;
    find.name = name.c_str();
    auto found = binary_find(byName.begin(), byName.end(), find);
    if(found != byName.end())
    {
        do
        {
            if(find.cmp(*found, find, caseSensitive) == 0)
            {
                foundIndex = *found;
                return true;
            }
            ++found;
        }
        while(found != byName.end() && find.cmp(find, *found, false) == 0);
    }
    return false;
}

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
