#include "MemoryMapView.h"

MemoryMapView::MemoryMapView(StdTable *parent) : StdTable(parent)
{
    enableMultiSelection(false);

    int charwidth=QFontMetrics(this->font()).width(QChar(' '));

    addColumnAt(8+charwidth*2*sizeof(uint_t), false); //addr
    addColumnAt(8+charwidth*2*sizeof(uint_t), false); //size
    addColumnAt(8+charwidth*32, false); //module
    addColumnAt(8+charwidth*3, false);
    addColumnAt(8+charwidth*5, false);
    addColumnAt(8+charwidth*5, false);
    addColumnAt(100, false);


    //setRowCount(100);


    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(stateChangedSlot(DBGSTATE)));
}

QString MemoryMapView::getProtectionString(DWORD Protect)
{
    QString wS;
    switch(Protect & 0xFF)
    {
    case PAGE_EXECUTE:
        wS = QString("E---");
        break;
    case PAGE_EXECUTE_READ:
        wS = QString("ER--");
        break;
    case PAGE_EXECUTE_READWRITE:
        wS = QString("ERW-");
        break;
    case PAGE_EXECUTE_WRITECOPY:
        wS = QString("ERWC");
        break;
    case PAGE_NOACCESS:
        wS = QString("----");
        break;
    case PAGE_READONLY:
        wS = QString("-R--");
        break;
    case PAGE_READWRITE:
        wS = QString("-RW-");
        break;
    case PAGE_WRITECOPY:
        wS = QString("-RWC");
        break;
    }
    if(Protect&PAGE_GUARD)
        wS+=QString("G");
    else
        wS+=QString("-");
    return wS;
}

void MemoryMapView::stateChangedSlot(DBGSTATE state)
{
    if(state == paused)
    {
        MEMMAP wMemMapStruct;
        int wI;

        memset(&wMemMapStruct, 0, sizeof(MEMMAP));


        Bridge::getBridge()->getMemMapFromDbg(&wMemMapStruct);

        //qDebug() << "count " << wMemMapStruct.count;

        setRowCount(wMemMapStruct.count);

        for(wI = 0; wI < wMemMapStruct.count; wI++)
        {
            QString wS;
            MEMORY_BASIC_INFORMATION wMbi = (wMemMapStruct.page)[wI].mbi;

            // Base address
            wS = QString("%1").arg((uint_t)wMbi.BaseAddress, sizeof(uint_t)*2, 16, QChar('0')).toUpper();
            setCellContent(wI, 0, wS);

            // Size
            wS = QString("%1").arg((uint_t)wMbi.RegionSize, sizeof(uint_t)*2, 16, QChar('0')).toUpper();
            setCellContent(wI, 1, wS);

            // Module Name
            char newMod[17]="";
            memcpy(newMod, (wMemMapStruct.page)[wI].mod, 16);
            wS = QString(newMod);
            setCellContent(wI, 2, wS);

            // State
            switch(wMbi.State)
            {
                case MEM_FREE:
                    wS = QString("FREE");
                    break;
                case MEM_COMMIT:
                    wS = QString("COMM");
                    break;
                case MEM_RESERVE:
                    wS = QString("RESV");
                    break;
                default:
                    wS = QString("????");
            }
            setCellContent(wI, 3, wS);

            // Type
            switch(wMbi.Type)
            {
                case MEM_IMAGE:
                    wS = QString("IMG");
                    break;
                case MEM_MAPPED:
                    wS = QString("MAP");
                    break;
                case MEM_PRIVATE:
                    wS = QString("PRV");
                    break;
                default:
                    wS = QString("N/A");
                    break;
            }
            setCellContent(wI, 3, wS);

            // current access protection
            wS=getProtectionString(wMbi.Protect);
            setCellContent(wI, 4, wS);

            // allocation protection
            wS=getProtectionString(wMbi.AllocationProtect);
            setCellContent(wI, 5, wS);

        }

        if(wMemMapStruct.page != 0)
        {
            Bridge::getBridge()->Free(wMemMapStruct.page);
        }
    }

}
