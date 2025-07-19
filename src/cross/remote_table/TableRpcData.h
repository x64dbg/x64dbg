#pragma once

#include "Types.h"
#include "json.hpp"

#include <vector>
#include <string>

struct TableRequest
{
    static constexpr const char* method = "table";

    duint offset = 0;
    duint lines = 0;
    dsint scroll = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TableRequest, offset, lines, scroll);

struct TableResponse
{
    static constexpr const char* method = "table";

    std::vector<std::vector<std::string>> rows;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TableResponse, rows);


//QString jsonRpcRequest(const QString& method, )
