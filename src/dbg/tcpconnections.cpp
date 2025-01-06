#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// NOTE: This is a hack to get Windows 7 definitions in this file
#ifndef WIN32_NO_STATUS
#define WIN32_NO_STATUS
#endif
#include <WS2tcpip.h>
#undef _WIN32_WINNT
#undef WINVER
#undef _WIN32_IE
#include "tcpconnections.h"
#include "IPHlpApi.h"

static const char* TcpStateToString(DWORD State)
{
    switch(State)
    {
    case MIB_TCP_STATE_CLOSED:
        return "CLOSED";
    case MIB_TCP_STATE_LISTEN:
        return "LISTEN";
    case MIB_TCP_STATE_SYN_SENT:
        return "SYN-SENT";
    case MIB_TCP_STATE_SYN_RCVD:
        return "SYN-RECEIVED";
    case MIB_TCP_STATE_ESTAB:
        return "ESTABLISHED";
    case MIB_TCP_STATE_FIN_WAIT1:
        return "FIN-WAIT-1";
    case MIB_TCP_STATE_FIN_WAIT2:
        return "FIN-WAIT-2";
    case MIB_TCP_STATE_CLOSE_WAIT:
        return "CLOSE-WAIT";
    case MIB_TCP_STATE_CLOSING:
        return "CLOSING";
    case MIB_TCP_STATE_LAST_ACK:
        return "LAST-ACK";
    case MIB_TCP_STATE_TIME_WAIT:
        return "TIME-WAIT";
    case MIB_TCP_STATE_DELETE_TCB:
        return "DELETE-TCB";
    default:
        return "UNKNOWN";
    }
}

typedef ULONG(WINAPI* GETTCPTABLE2)(PMIB_TCPTABLE2 TcpTable, PULONG SizePointer, BOOL Order);
typedef ULONG(WINAPI* GETTCP6TABLE2)(PMIB_TCP6TABLE2 TcpTable, PULONG SizePointer, BOOL Order);
typedef PCTSTR(WSAAPI* INETNTOPW)(INT Family, PVOID pAddr, wchar_t* pStringBuf, size_t StringBufSize);

bool TcpEnumConnections(duint pid, std::vector<TCPCONNECTIONINFO> & connections)
{
    // The following code is modified from code sample at MSDN.GetTcpTable2
    static auto hIpHlp = LoadLibraryW(L"iphlpapi.dll");
    if(!hIpHlp)
        return false;

    // To ensure WindowsXP compatibility we won't link them statically
    static auto GetTcpTable2 = GETTCPTABLE2(GetProcAddress(hIpHlp, "GetTcpTable2"));
    static auto GetTcp6Table2 = GETTCP6TABLE2(GetProcAddress(hIpHlp, "GetTcp6Table2"));
    static auto InetNtopW = INETNTOPW(GetProcAddress(GetModuleHandleW(L"ws2_32.dll"), "InetNtopW"));
    if(!InetNtopW)
        return false;

    TCPCONNECTIONINFO info;
    wchar_t AddrBuffer[TCP_ADDR_SIZE] = L"";

    if(GetTcpTable2)
    {
        ULONG ulSize = 0;
        // Make an initial call to GetTcpTable2 to get the necessary size into the ulSize variable
        if(GetTcpTable2(nullptr, &ulSize, TRUE) == ERROR_INSUFFICIENT_BUFFER)
        {
            Memory<MIB_TCPTABLE2*> pTcpTable(ulSize);
            // Make a second call to GetTcpTable2 to get the actual data we require
            if(GetTcpTable2(pTcpTable(), &ulSize, TRUE) == NO_ERROR)
            {
                for(auto i = 0; i < int(pTcpTable()->dwNumEntries); i++)
                {
                    auto & entry = pTcpTable()->table[i];
                    if(entry.dwOwningPid != pid)
                        continue;

                    info.State = entry.dwState;
                    strcpy_s(info.StateText, TcpStateToString(info.State));

                    struct in_addr IpAddr;
                    IpAddr.S_un.S_addr = u_long(entry.dwLocalAddr);
                    InetNtopW(AF_INET, &IpAddr, AddrBuffer, TCP_ADDR_SIZE);
                    strcpy_s(info.LocalAddress, StringUtils::Utf16ToUtf8(AddrBuffer).c_str());
                    info.LocalPort = ntohs(u_short(entry.dwLocalPort));

                    IpAddr.S_un.S_addr = u_long(entry.dwRemoteAddr);
                    InetNtopW(AF_INET, &IpAddr, AddrBuffer, TCP_ADDR_SIZE);
                    strcpy_s(info.RemoteAddress, StringUtils::Utf16ToUtf8(AddrBuffer).c_str());
                    info.RemotePort = ntohs(u_short(entry.dwRemotePort));

                    connections.push_back(info);
                }
            }
        }
    }

    if(GetTcp6Table2)
    {
        ULONG ulSize = 0;
        // Make an initial call to GetTcp6Table2 to get the necessary size into the ulSize variable
        if(GetTcp6Table2(nullptr, &ulSize, TRUE) == ERROR_INSUFFICIENT_BUFFER)
        {
            Memory<MIB_TCP6TABLE2*> pTcp6Table(ulSize);
            // Make a second call to GetTcpTable2 to get the actual data we require
            if(GetTcp6Table2(pTcp6Table(), &ulSize, TRUE) == NO_ERROR)
            {
                for(auto i = 0; i < int(pTcp6Table()->dwNumEntries); i++)
                {
                    auto & entry = pTcp6Table()->table[i];
                    if(entry.dwOwningPid != pid)
                        continue;

                    info.State = entry.State;
                    strcpy_s(info.StateText, TcpStateToString(info.State));

                    InetNtopW(AF_INET6, &entry.LocalAddr, AddrBuffer, TCP_ADDR_SIZE);
                    sprintf_s(info.LocalAddress, "[%s]", StringUtils::Utf16ToUtf8(AddrBuffer).c_str());
                    info.LocalPort = ntohs(u_short(entry.dwLocalPort));

                    InetNtopW(AF_INET6, &entry.RemoteAddr, AddrBuffer, TCP_ADDR_SIZE);
                    sprintf_s(info.RemoteAddress, "[%s]", StringUtils::Utf16ToUtf8(AddrBuffer).c_str());
                    info.RemotePort = ntohs(u_short(entry.dwRemotePort));

                    connections.push_back(info);
                }
            }
        }
    }
    return true;
}
