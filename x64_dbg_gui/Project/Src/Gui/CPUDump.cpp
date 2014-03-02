#include "CPUDump.h"

CPUDump::CPUDump(QWidget *parent) : HexDump(parent)
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 8;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendDescriptor(8+charwidth*23, "Hex", false, wColDesc);

    wColDesc.isData = true; //ascii byte
    wColDesc.itemCount = 8;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(8+charwidth*15, "ASCII", false, wColDesc);

    wColDesc.isData = true; //float qword
    wColDesc.itemCount = 1;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = DoubleQword;
    appendDescriptor(8+charwidth*23, "Double", false, wColDesc);

    wColDesc.isData = true; //void*
    wColDesc.itemCount = 1;
#ifdef _WIN64
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
#else
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
#endif
    appendDescriptor(8+charwidth*2*sizeof(uint_t), "void*", false, wColDesc);

    wColDesc.isData = false; //comments
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "Comments", false, wColDesc);

    connect(Bridge::getBridge(), SIGNAL(dumpAt(int_t)), this, SLOT(printDumpAt(int_t)));
}

QString CPUDump::printNonData(int col, int_t wRva, ColumnDescriptor_t descriptor, MemoryPage* memPage)
{
    QString wStr = "";
    uint_t data=0;
    memPage->readOriginalMemory((byte_t*)&data, wRva, sizeof(uint_t));
    char label_text[MAX_LABEL_SIZE]="";
    if(DbgGetLabelAt(data, SEG_DEFAULT, label_text))
        wStr+=QString(label_text);// + QString("%1").arg(data, sizeof(uint_t)*2, 16, QChar('0')).toUpper();
    return wStr;
}
