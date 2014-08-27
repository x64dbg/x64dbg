/* plugin:   (StaticAnalysis) for x64dbg <http://www.x64dbg.com>
 * author:   tr4ceflow@gmail.com <http://blog.traceflow.com>
 * license:  GLPv3
 */
#pragma once
#include "meta.h"
#include <list>


namespace fa
{

typedef std::list<FunctionInfo_t> FunctionInfoList;

class FunctionDB
{
private:
    bool mValid;  // "database" ok ?
public:
    FunctionDB(void);
    ~FunctionDB(void);

    const bool ok() const;

    FunctionInfo_t find(std::string name);

    FunctionInfoList mInfo;


};

}