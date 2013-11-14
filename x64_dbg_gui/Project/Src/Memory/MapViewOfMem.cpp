#include "MapViewOfMem.h"


MapViewOfMem::MapViewOfMem()
{
    mSelectedData.fromIndex = -1;
    mSelectedData.toIndex = -1;
}

MapViewOfMem::MapViewOfMem(QString file)
{
    MemDataCacheStruct_t data;
    memset(&data, 0, sizeof(MemDataCacheStruct_t));
    data.memDataCachePtr=new QVector<byte_t>(0);
    mGuiMemDataCache = data;

    mSelectedData.fromIndex = -1;
    mSelectedData.toIndex = -1;

    //qDebug() << "MapViewOfMem() Load data from file.";

    QFile wFile(file);

    if(wFile.open(QIODevice::ReadOnly) == false)
         //qDebug() << "File has not been opened.";

    mData = wFile.readAll();
    //qDebug() << "Size: " << mData.size();

    if(mData.size() == 0)
    {
        //qDebug() << "No Data";
    }

    mSize = mData.size();
    mBase = 0x00401000;
}


MapViewOfMem::MapViewOfMem(uint_t startAddress , uint_t size)
{
    mStartAddress = startAddress;
    mEndAddress = startAddress + size - 1;
    mSize = size;
}

MapViewOfMem::~MapViewOfMem()
{

}

byte_t MapViewOfMem::readByte(uint_t rva)
{
    return mData.data()[rva];
}

uint_t MapViewOfMem::size()
{
    return mSize;
}

unsigned char* MapViewOfMem::data()
{
    return (unsigned char*)(mData.data());
}


Selection_t MapViewOfMem::getSelection()
{
    return mSelectedData;
}

void MapViewOfMem::setSelection(Selection_t sel)
{
    mSelectedData = sel;
}

uint_t MapViewOfMem::getBase()
{
    return mBase;
}


byte_t* MapViewOfMem::getDataPtrForGui(uint_t rva, uint_t maxNbrOfBytesToRead, uint_t newCacheSize)
{
    byte_t* wBytePtr = 0;

    if(maxNbrOfBytesToRead > 0)
    {
        // Bound maxNbrOfBytesToRead to the max value it can take
        if(maxNbrOfBytesToRead > (this->size() - rva))
            maxNbrOfBytesToRead = this->size() - rva;

        if((mGuiMemDataCache.isInit == true) && (rva >= mGuiMemDataCache.rva) && ((rva + (uint_t)maxNbrOfBytesToRead) <= (mGuiMemDataCache.rva + (uint_t)mGuiMemDataCache.memDataCacheSize)))
        {
            // Cache Success
            wBytePtr = mGuiMemDataCache.memDataCachePtr->data() + (rva - mGuiMemDataCache.rva);
        }
        else
        {
            // Cache Miss
            mGuiMemDataCache.memDataCacheSize = newCacheSize;
            mGuiMemDataCache.memDataCachePtr->resize(newCacheSize);
            mGuiMemDataCache.rva = rva;
            wBytePtr = mGuiMemDataCache.memDataCachePtr->data();
            // TODO: Fill cache
            for(uint_t wI = 0; wI < newCacheSize; wI++)
            {
                wBytePtr[wI] = readByte(rva + (uint_t)wI);
            }
            mGuiMemDataCache.isInit = true;
        }
    }

    return wBytePtr;
}














