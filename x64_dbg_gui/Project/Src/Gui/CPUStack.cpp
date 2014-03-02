#include "CPUStack.h"

CPUStack::CPUStack(QWidget *parent) : HexDump(parent)
{
    setShowHeader(false);
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

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

QString CPUStack::printNonData(int col, int_t wRva, ColumnDescriptor_t descriptor, MemoryPage* memPage)
{
    return "Stack comment";
}
