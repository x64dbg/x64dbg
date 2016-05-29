#pragma once

#include "_global.h"
#include "_dbgfunctions.h"

bool TcpEnumConnections(duint pid, std::vector<TCPCONNECTIONINFO> & connections);
