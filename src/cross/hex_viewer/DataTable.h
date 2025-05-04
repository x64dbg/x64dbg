#pragma once

#include <vector>
#include <functional>

#include <BasicView/StdTable.h>

class DataTable : public StdTable
{
public:
    explicit DataTable(QWidget* parent = nullptr);

public slots:
    void selectionChanged(duint start, duint end);

private:
    bool read(void* destination, duint addr, duint size);

    template<class T>
    QString primitive(duint addr)
    {
        T data = {};
        if(!read(&data, addr, sizeof(data)))
        {
            return "???";
        }
        return QString("%1").arg(data);
    }

    enum Columns {
        ColType,
        ColValue,
    };

    using DataFn = std::function<QString(duint, duint)>;

    std::vector<std::pair<QString, DataFn>> mTypes;
};
