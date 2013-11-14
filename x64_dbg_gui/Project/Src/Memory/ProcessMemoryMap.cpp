#include "ProcessMemoryMap.h"


ProcessMemoryMap::ProcessMemoryMap(QString fileName, QObject *parent) : QObject(parent)
{

    STARTUPINFO si;
    memset(&si, 0, sizeof(STARTUPINFO));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(PROCESS_INFORMATION));

    LPCTSTR target = TEXT("UnPackMe.exe");

    if(!CreateProcess((LPCTSTR)target, 0, NULL,NULL,0,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi))
    {
        //qDebug() << "CreateProcess failed (" << GetLastError() << ").\n";
    }
    else
    {
        mhProcess = pi.hProcess;

        QWidget* widget = new QWidget();
        widget->show();

        Sleep(800);

        printVirtualMemoryMap(BruteForceProcessMemory());
    }


}




QList<MEMORY_BASIC_INFORMATION> ProcessMemoryMap::BruteForceProcessMemory()
{
    QList<MEMORY_BASIC_INFORMATION> wMemoryRegionsList;
    MEMORY_BASIC_INFORMATION wMemInfo;
    uint_t wVirtualAddress = 0;
    uint_t wUpperLimit = 0x80000000;//0x000007FFFFFFFFFF;    // x64 User Space Limit
    SIZE_T wErr;

    // For each page in memory
    while(wVirtualAddress <= wUpperLimit)
    {
        // Query the page
        wErr = VirtualQueryEx(mhProcess, (LPCVOID)wVirtualAddress, &wMemInfo, sizeof(wMemInfo));

        // If VirtualQueryEx failed, try the next region

        if(wErr == 0)
        {
            wVirtualAddress += 0x1000;
        }
        else
        {
            wMemoryRegionsList.append(wMemInfo);
            wVirtualAddress += wMemInfo.RegionSize;
        }
    }

    return wMemoryRegionsList;
}




void ProcessMemoryMap::printVirtualMemoryMap(QList<MEMORY_BASIC_INFORMATION> parList)
{
    int wI;
    QString wStr = "";
    QString wTmpStr = "";
    MEMORY_BASIC_INFORMATION wMemInfo;

    // Header
    wTmpStr = "Address";
    wStr += wTmpStr + QString(" ").repeated(20 - wTmpStr.length());
    wStr += " | ";

    wTmpStr = "Size";
    wStr += wTmpStr + QString(" ").repeated(20 - wTmpStr.length());
    wStr += " | ";

    wTmpStr = "State";
    wStr += wTmpStr + QString(" ").repeated(10 - wTmpStr.length());
    wStr += " | ";

    wTmpStr = "Access";
    wStr += wTmpStr + QString(" ").repeated(30 - wTmpStr.length());
    wStr += " | ";

    wTmpStr = "Type";
    wStr += wTmpStr + QString(" ").repeated(10 - wTmpStr.length());

    //qDebug() << wStr;

    wStr = "-------------------------------------------------------------------------------------------------";
    //qDebug() << wStr;


    for(wI = 0; wI < parList.size(); wI++)
    {
        wStr = "";
        wMemInfo = parList.at(wI);

        // Base address
        wTmpStr = "0x" + QString("%1").arg((uint_t)wMemInfo.BaseAddress, 16, 16, QChar('0')).toUpper();
        wStr += wTmpStr + QString(" ").repeated(20 - wTmpStr.length());
        wStr += " | ";

        // Size
        wTmpStr = "0x" + QString("%1").arg((uint_t)wMemInfo.RegionSize, 16, 16, QChar('0')).toUpper();
        wStr += wTmpStr + QString(" ").repeated(20 - wTmpStr.length());
        wStr += " | ";

        // State
        switch(wMemInfo.State)
        {
            case MEM_FREE:
                wTmpStr = QString("Free");
                break;
            case MEM_COMMIT:
                wTmpStr = QString("Commited");
                break;
            case MEM_RESERVE:
                wTmpStr = QString("Reserved");
                break;
            default:
                wTmpStr = QString("N/A");
        }
        wStr += wTmpStr + QString(" ").repeated(10 - wTmpStr.length());
        wStr += " | ";

        // Access
        if(wMemInfo.State != MEM_COMMIT)
        {
            wTmpStr = QString("N/A");
            wStr += wTmpStr + QString(" ").repeated(30 - wTmpStr.length());
            wStr += " | ";
        }
        else
        {
            switch(wMemInfo.Protect & 0xFF)
            {
                case PAGE_EXECUTE:
                    wTmpStr = QString("Execute");
                    break;
                case PAGE_EXECUTE_READ:
                    wTmpStr = QString("Execute/Read");
                    break;
                case PAGE_EXECUTE_READWRITE:
                    wTmpStr = QString("Execute/Read/Write");
                    break;
                case PAGE_NOACCESS:
                    wTmpStr = QString("No Access");
                    break;
                case PAGE_READONLY:
                    wTmpStr = QString("Read");
                    break;
                case PAGE_READWRITE:
                    wTmpStr = QString("Read/Write");
                    break;
                case PAGE_WRITECOPY:
                    wTmpStr = QString("Copy on Write");
                    break;
                case PAGE_EXECUTE_WRITECOPY:
                    wTmpStr = QString("Execute/Copy on Write");
                    break;
            }

            switch(wMemInfo.Protect & 0xFF00)
            {
                case PAGE_GUARD:
                    wTmpStr += QString(" + Guard");
            }

            wStr += wTmpStr + QString(" ").repeated(30 - wTmpStr.length());
            wStr += " | ";
        }

        // Type
        switch(wMemInfo.Type)
        {
            case MEM_IMAGE:
                wTmpStr = QString("Image");
                break;
            case MEM_MAPPED:
                wTmpStr = QString("Mapped");
                break;
            case MEM_PRIVATE:
                wTmpStr = QString("Private");
                break;
            default:
                wTmpStr = QString("N/A");
                break;
        }
        wStr += wTmpStr + QString(" ").repeated(10 - wTmpStr.length());


        //qDebug() << wStr;
    }
}












