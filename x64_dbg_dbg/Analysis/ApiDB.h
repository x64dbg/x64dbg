#pragma once
#include "Meta.h"
#include <list>

namespace tr4ce
{
class ApiDB
{
private:
    bool mValid;  // "database" ok ?
public:
    ApiDB(void);
    ~ApiDB(void);

    const bool ok() const;

    FunctionInfo_t find(std::string name);

    std::list<FunctionInfo_t> mInfo;


};

};