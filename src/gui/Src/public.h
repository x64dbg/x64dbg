#ifndef PUBLIC_H
#define PUBLIC_H
#include "Imports.h"
#include <QApplication>
#include <Tlhelp32.h>
extern uint g_pid;
extern QVector<MODULEENTRY32> g_moduleArr;
uint GetProcessModuleAddress(uint pid,QString moduleName);
uint GetModuleFromAddr(uint pid,void* p);
int EnumProcessModule(uint pid,QVector<MODULEENTRY32>& moduleArr);
bool GetModuleInfoFromAddr(ULONG_PTR addr,MODULEENTRY32** moduleInfo);
#endif // PUBLIC_H

