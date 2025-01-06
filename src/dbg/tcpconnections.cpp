#include "tcpconnections.h"
#include <WS2tcpip.h>

typedef enum
{
    DBG_MIB_TCP_STATE_CLOSED = 1,
    DBG_MIB_TCP_STATE_LISTEN = 2,
    DBG_MIB_TCP_STATE_SYN_SENT = 3,
    DBG_MIB_TCP_STATE_SYN_RCVD = 4,
    DBG_MIB_TCP_STATE_ESTAB = 5,
    DBG_MIB_TCP_STATE_FIN_WAIT1 = 6,
    DBG_MIB_TCP_STATE_FIN_WAIT2 = 7,
    DBG_MIB_TCP_STATE_CLOSE_WAIT = 8,
    DBG_MIB_TCP_STATE_CLOSING = 9,
    DBG_MIB_TCP_STATE_LAST_ACK = 10,
    DBG_MIB_TCP_STATE_TIME_WAIT = 11,
    DBG_MIB_TCP_STATE_DELETE_TCB = 12,
} DBG_MIB_TCP_STATE;

static const char* TcpStateToString(unsigned int State)
{
    switch(State)
    {
    case DBG_MIB_TCP_STATE_CLOSED:
        return "CLOSED";
    case DBG_MIB_TCP_STATE_LISTEN:
        return "LISTEN";
    case DBG_MIB_TCP_STATE_SYN_SENT:
        return "SYN-SENT";
    case DBG_MIB_TCP_STATE_SYN_RCVD:
        return "SYN-RECEIVED";
    case DBG_MIB_TCP_STATE_ESTAB:
        return "ESTABLISHED";
    case DBG_MIB_TCP_STATE_FIN_WAIT1:
        return "FIN-WAIT-1";
    case DBG_MIB_TCP_STATE_FIN_WAIT2:
        return "FIN-WAIT-2";
    case DBG_MIB_TCP_STATE_CLOSE_WAIT:
        return "CLOSE-WAIT";
    case DBG_MIB_TCP_STATE_CLOSING:
        return "CLOSING";
    case DBG_MIB_TCP_STATE_LAST_ACK:
        return "LAST-ACK";
    case DBG_MIB_TCP_STATE_TIME_WAIT:
        return "TIME-WAIT";
    case DBG_MIB_TCP_STATE_DELETE_TCB:
        return "DELETE-TCB";
    default:
        return "UNKNOWN";
    }
}

typedef enum
{
    DbgTcpConnectionOffloadStateInHost,
    DbgTcpConnectionOffloadStateOffloading,
    DbgTcpConnectionOffloadStateOffloaded,
    DbgTcpConnectionOffloadStateUploading,
    DbgTcpConnectionOffloadStateMax
} DBG_TCP_CONNECTION_OFFLOAD_STATE, *PTCP_CONNECTION_OFFLOAD_STATE;

typedef struct _DBG_MIB_TCPROW2
{
    DWORD                            dwState;
    DWORD                            dwLocalAddr;
    DWORD                            dwLocalPort;
    DWORD                            dwRemoteAddr;
    DWORD                            dwRemotePort;
    DWORD                            dwOwningPid;
    DBG_TCP_CONNECTION_OFFLOAD_STATE dwOffloadState;
} DBG_MIB_TCPROW2, *PDBG_MIB_TCPROW2;

typedef struct _DBG_MIB_TCPTABLE2
{
    DWORD           dwNumEntries;
    DBG_MIB_TCPROW2 table[1];
} DBG_MIB_TCPTABLE2, *PDBG_MIB_TCPTABLE2;

typedef struct _DBG_IN6_ADDR
{
    union
    {
        UCHAR       Byte[16];
        USHORT      Word[8];
    } u;
} DBG_IN6_ADDR, * PDBG_IN6_ADDR, FAR* LPDBG_IN6_ADDR;

typedef struct _MIB_TCP6ROW2
{
    DBG_IN6_ADDR                     LocalAddr;
    DWORD                            dwLocalScopeId;
    DWORD                            dwLocalPort;
    DBG_IN6_ADDR                     RemoteAddr;
    DWORD                            dwRemoteScopeId;
    DWORD                            dwRemotePort;
    DWORD                            State;
    DWORD                            dwOwningPid;
    DBG_TCP_CONNECTION_OFFLOAD_STATE dwOffloadState;
} DBG_MIB_TCP6ROW2, *PDBG_MIB_TCP6ROW2;

typedef struct _DBG_MIB_TCP6TABLE2
{
    DWORD        dwNumEntries;
    DBG_MIB_TCP6ROW2 table[1];
} DBG_MIB_TCP6TABLE2, *PDBG_MIB_TCP6TABLE2;

typedef ULONG(WINAPI* GETTCPTABLE2)(PDBG_MIB_TCPTABLE2 TcpTable, PULONG SizePointer, BOOL Order);
typedef ULONG(WINAPI* GETTCP6TABLE2)(PDBG_MIB_TCP6TABLE2 TcpTable, PULONG SizePointer, BOOL Order);
typedef PCTSTR(WINAPI* INETNTOPW)(INT Family, PVOID pAddr, wchar_t* pStringBuf, size_t StringBufSize);

bool TcpEnumConnections(duint pid, std::vector<TCPCONNECTIONINFO> & connections)
{
    // The following code is modified from code sample at MSDN.GetTcpTable2
    static auto hIpHlp = LoadLibraryW(L"iphlpapi.dll");
    if(!hIpHlp)
        return false;

    // To ensure WindowsXP compatibility we won't link them statically
    static auto pGetTcpTable2 = GETTCPTABLE2(GetProcAddress(hIpHlp, "GetTcpTable2"));
    static auto pGetTcp6Table2 = GETTCP6TABLE2(GetProcAddress(hIpHlp, "GetTcp6Table2"));
    static auto pInetNtopW = INETNTOPW(GetProcAddress(GetModuleHandleW(L"ws2_32.dll"), "InetNtopW"));
    if(!pInetNtopW)
        return false;

    TCPCONNECTIONINFO info;
    wchar_t AddrBuffer[TCP_ADDR_SIZE] = L"";

    if(pGetTcpTable2)
    {
        ULONG ulSize = 0;
        // Make an initial call to GetTcpTable2 to get the necessary size into the ulSize variable
        if(pGetTcpTable2(nullptr, &ulSize, TRUE) == ERROR_INSUFFICIENT_BUFFER)
        {
            Memory<DBG_MIB_TCPTABLE2*> pTcpTable(ulSize);
            // Make a second call to GetTcpTable2 to get the actual data we require
            if(pGetTcpTable2(pTcpTable(), &ulSize, TRUE) == NO_ERROR)
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
                    pInetNtopW(AF_INET, &IpAddr, AddrBuffer, TCP_ADDR_SIZE);
                    strcpy_s(info.LocalAddress, StringUtils::Utf16ToUtf8(AddrBuffer).c_str());
                    info.LocalPort = ntohs(u_short(entry.dwLocalPort));

                    IpAddr.S_un.S_addr = u_long(entry.dwRemoteAddr);
                    pInetNtopW(AF_INET, &IpAddr, AddrBuffer, TCP_ADDR_SIZE);
                    strcpy_s(info.RemoteAddress, StringUtils::Utf16ToUtf8(AddrBuffer).c_str());
                    info.RemotePort = ntohs(u_short(entry.dwRemotePort));

                    connections.push_back(info);
                }
            }
        }
    }

    if(pGetTcp6Table2)
    {
        ULONG ulSize = 0;
        // Make an initial call to GetTcp6Table2 to get the necessary size into the ulSize variable
        if(pGetTcp6Table2(nullptr, &ulSize, TRUE) == ERROR_INSUFFICIENT_BUFFER)
        {
            Memory<DBG_MIB_TCP6TABLE2*> pTcp6Table(ulSize);
            // Make a second call to GetTcpTable2 to get the actual data we require
            if(pGetTcp6Table2(pTcp6Table(), &ulSize, TRUE) == NO_ERROR)
            {
                for(auto i = 0; i < int(pTcp6Table()->dwNumEntries); i++)
                {
                    auto & entry = pTcp6Table()->table[i];
                    if(entry.dwOwningPid != pid)
                        continue;

                    info.State = entry.State;
                    strcpy_s(info.StateText, TcpStateToString(info.State));

                    pInetNtopW(AF_INET6, &entry.LocalAddr, AddrBuffer, TCP_ADDR_SIZE);
                    sprintf_s(info.LocalAddress, "[%s]", StringUtils::Utf16ToUtf8(AddrBuffer).c_str());
                    info.LocalPort = ntohs(u_short(entry.dwLocalPort));

                    pInetNtopW(AF_INET6, &entry.RemoteAddr, AddrBuffer, TCP_ADDR_SIZE);
                    sprintf_s(info.RemoteAddress, "[%s]", StringUtils::Utf16ToUtf8(AddrBuffer).c_str());
                    info.RemotePort = ntohs(u_short(entry.dwRemotePort));

                    connections.push_back(info);
                }
            }
        }
    }
    return true;
}
