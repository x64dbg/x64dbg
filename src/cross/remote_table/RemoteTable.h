#pragma once

#include <BasicView/AbstractStdTable.h>
#include "MagicMenu.h"
#include "JsonRpcClient.h"
#include "TableRpcData.h"

#include <QWebSocket>

class RemoteTable : public AbstractStdTable, public MagicMenu<RemoteTable>
{
    Q_OBJECT

public:
    explicit RemoteTable(QWidget* parent = nullptr);

protected:
    QString getCellContent(duint row, duint col) override;
    bool isValidIndex(duint row, duint col) override;
    void sortRows(duint column, bool ascending) override;
    duint sliderMovedHook(QScrollBar::SliderAction action, duint value, dsint delta) override;
    void prepareData() override;

private:
    enum ColumnIndex
    {
        ColAddress,
        ColData,
    };

    std::vector<std::vector<std::string>> mRemoteData;
    std::chrono::time_point<std::chrono::system_clock> mLastPrepare;

    QWebSocket mSocket;
    JsonRpcClient mRpc;

    TableRequest mCurrentRequest;
    bool mCurrentSent = false;

    TableRequest mNextRequest;
    bool mNextRequired = false;

    void handleTableResponse(const TableResponse & response);
    void setupMenu();
    void setupConnection();
};
