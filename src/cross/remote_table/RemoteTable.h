#pragma once

#include <BasicView/AbstractStdTable.h>
#include "MagicMenu.h"
#include "JsonRpcClient.h"
#include "TableRpcData.h"
#include "OverlayFrame.h"

#include <QWebSocket>
#include <QWidget>
#include <deque>

class RemoteTable : public AbstractStdTable, public MagicMenu<RemoteTable>
{
    Q_OBJECT

public:
    explicit RemoteTable(QWidget* parent = nullptr);

protected:
    QString getCellContent(duint row, duint col) override;
    duint getCellUserdata(duint row, duint column) override;
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
    std::chrono::time_point<std::chrono::system_clock> mCurrentSentTime;

    TableRequest mNextRequest;
    bool mNextRequired = false;

    OverlayFrame* mOverlay = nullptr;

    std::deque<uint64_t> mResponseTimes;
    uint64_t mMinResponseTime = 0;
    uint64_t mMaxResponseTime = 100;
    uint64_t mAvgResponseTime = 100;
    uint64_t mMedResponseTime = 100;

    void handleTableResponse(const TableResponse & response);
    void setupMenu();
    void setupConnection();
};
