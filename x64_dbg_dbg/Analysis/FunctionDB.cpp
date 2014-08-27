/* plugin:   (StaticAnalysis) for x64dbg <http://www.x64dbg.com>
 * author:   tr4ceflow@gmail.com <http://blog.traceflow.com>
 * license:  GLPv3
 */
#include "FunctionDB.h"
#include <fstream>
#include <sstream>
#include <vector>
#include "../_global.h"
#include "../console.h"
#include "Meta.h"

namespace fa
{
// http://stackoverflow.com/a/22923239/1392416
std::vector<std::string> split(const std::string & string, const char* del)
{
    size_t first = 0, second = 0;
    size_t end = string.size();
    size_t len = strlen(del);
    std::vector<std::string> tokens;
    while((second = string.find(del, first)) != (std::string::npos))
    {
        size_t dif = second - first;
        if(dif)
        {
            tokens.push_back(string.substr(first, dif));
        }
        first = second + len;
    }
    if(first != end)
    {
        tokens.push_back(string.substr(first));
    }
    return tokens;
}


FunctionDB::FunctionDB(void)
{
    unsigned int i = 0;
    mValid = true;

    std::ifstream helpFile;
    std::string rawLine;
    helpFile.open("api.dat");
    if(!helpFile)
    {
        dputs("[StaticAnalysis] api help file not found ...");
    }
    else
    {
        dputs("[StaticAnalysis] load api help file  ...");
        while(!helpFile.eof())
        {
            helpFile >> rawLine;
            std::vector<std::string> tokens = split(rawLine, ";");

            if(tokens.size() > 3)
            {
                FunctionInfo_t f;
                f.DLLName = tokens.at(0);
                f.ReturnType = tokens.at(1);
                f.Name = tokens.at(2);

                for(int j = 3; j < tokens.size() - 1; j += 2)
                {
                    ArgumentInfo_t a;
                    a.Type = tokens.at(j);
                    a.Name = tokens.at(j + 1);
                    f.Arguments.push_back(a);
                }


                mInfo.push_back(f);

                i++;
            }



        }


    }

    dprintf("[StaticAnalysis] loaded %i functions signatures from helpfile\n", i);
    helpFile.close();
}





FunctionDB::~FunctionDB(void)
{
}

FunctionInfo_t FunctionDB::find(std::string name)
{
    if(name[0] == '&')
        name.erase(0, 1);
    FunctionInfo_t f;
    f.invalid = true;

    //_plugin_logprintf("[StaticAnalysis:IntermodularCalls] search data %s \n",name.c_str() );
    std::list<FunctionInfo_t>::iterator it = mInfo.begin();

    while(it != mInfo.end())
    {
        if(it->Name == name)
        {
            f = *it;
            f.invalid = false;
            break;
        }

        it++;
    }



    return f;
}

const bool FunctionDB::ok() const
{
    return mValid;
}
}