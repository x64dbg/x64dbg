#include "DataTable.h"
#include "Bridge.h"

#include <QDebug>

DataTable::DataTable(QWidget* parent)
    : StdTable(parent)
{
    auto type = [this](const QString & name, DataFn fn)
    {
        mTypes.emplace_back(name, std::move(fn));
    };

    type("Address", [this](duint start, duint end)
    {
        auto size = end - start + 1;
        if(size > 1)
        {
            return QString("0x%1 - 0x%2 (0x%3 bytes)").arg(ToHexString(start)).arg(ToHexString(end)).arg(ToHexString(size));
        }
        else
        {
            return "0x" + ToHexString(start);
        }
    });

    type("Binary (u8)", [this](duint start, duint end)
    {
        uint8_t data = 0;
        read(&data, start, sizeof(data));
        uint8_t mask = 0x80;
        QString result = "0b0";
        for(int i = 0; i < 8; i++)
        {
            if(data & mask)
                result += "1";
            else
                result += "0";
        }
        return result;
    });
    type("uint8_t", [this](duint start, duint end)
    {
        return primitive<uint8_t>(start);
    });
    type("int8_t", [this](duint start, duint end)
    {
        return primitive<int8_t>(start);
    });
    type("uint16_t", [this](duint start, duint end)
    {
        return primitive<uint16_t>(start);
    });
    type("int16_t", [this](duint start, duint end)
    {
        return primitive<int16_t>(start);
    });

    int width = 1;
    for(size_t i = 0; i < mTypes.size(); i++)
    {
        width = qMax(width, mTypes[i].first.length());
    }
    addColumnAt(getCharWidth() * width + 8, "Type", true);
    addColumnAt(0, "Value", false);

    setRowCount(mTypes.size());
    for(size_t i = 0; i < mTypes.size(); i++)
    {
        setCellContent(i, ColType, mTypes[i].first);
    }
}

void DataTable::selectionChanged(duint start, duint end)
{
    for(size_t i = 0; i < mTypes.size(); i++)
    {
        auto value = mTypes[i].second(start, end);
        setCellContent(i, ColValue, value);
    }
    reloadData();
}

bool DataTable::read(void* destination, duint addr, duint size)
{
    return DbgMemRead(addr, destination, size);
}
