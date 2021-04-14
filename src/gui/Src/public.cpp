#include "public.h"
#include <Psapi.h>
uint g_pid=0;
QVector<MODULEENTRY32> g_moduleArr;
#pragma comment(lib, "Psapi.lib")

uint GetProcessModuleAddress(uint pid,QString moduleName)
{
    uint moduleAddr=0;
    HANDLE hpro = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
        if (hpro == 0)
        {
            //printf("无法获取进程句柄");
            CloseHandle(hpro);
            return 0;
        }
        // 获取每一个模块加载基址
        HMODULE hModule[100] = {0};
        DWORD dwRet = 0;
        int num = 0;
        int bRet = ::EnumProcessModulesEx(hpro, (HMODULE *)(hModule), sizeof(hModule),&dwRet,NULL);
        if (bRet == 0)
        {
            //printf("EnumProcessModules");
            CloseHandle(hpro);
            return 0;
        }
        // 总模块个数
        num = dwRet/sizeof(HMODULE);
        //printf("总模块个数: %d\n",num);
        // 打印每一个模块加载基址
        TCHAR lpBaseName[100];
        QString mName;
        for(int i = 0;i<num;i++)
        {
             ZeroMemory(lpBaseName,sizeof (lpBaseName));
            ::GetModuleBaseName(hpro,hModule[i],lpBaseName,sizeof(lpBaseName));
            mName=QString::fromWCharArray(lpBaseName);
            if(QString::compare(moduleName, mName, Qt::CaseInsensitive)==0)
            {
                moduleAddr=(uint)hModule[i];
                break;
            }
            //printf("%-2d %-25s基址: 0x%p\n",i,lpBaseName,hModule[i]);
        }
        CloseHandle(hpro);
        return moduleAddr;
}

int EnumProcessModule(uint pid,QVector<MODULEENTRY32>& moduleArr)
{
    HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid);//上面获取了进程PID,下面使用即可.
        if (INVALID_HANDLE_VALUE == hSnapshot)
        {
            return -1;
        }
        MODULEENTRY32 mi;
        mi.dwSize = sizeof(MODULEENTRY32); //第一次使用必须初始化成员
        BOOL  bRet = ::Module32First(hSnapshot,&mi);
        while (bRet)
        {
            moduleArr.push_back(mi);
            bRet = ::Module32Next(hSnapshot,&mi);
        }
        ::CloseHandle(hSnapshot);
        return 0;
}

bool GetModuleInfoFromAddr(ULONG_PTR addr,MODULEENTRY32** ppModuleInfo)
{
    for(auto& module:g_moduleArr)
    {
        if(addr>=(ULONG_PTR)module.modBaseAddr && addr<=(ULONG_PTR)(module.modBaseAddr+module.modBaseSize))
        {
            *ppModuleInfo=&module;
            return true;
        }
    }
    return false;
}

uint GetModuleFromAddr(uint pid,void* p)
{
    MEMORY_BASIC_INFORMATION m = { 0 };
    HANDLE hpro = ::OpenProcess(PROCESS_ALL_ACCESS,FALSE, pid);
        if (hpro == 0)
        {
            //printf("无法获取进程句柄");
            CloseHandle(hpro);
            return 0;
        }
    VirtualQueryEx(hpro,p, &m, sizeof(MEMORY_BASIC_INFORMATION));
    ::CloseHandle(hpro);
    return (uint)m.AllocationBase;
}
