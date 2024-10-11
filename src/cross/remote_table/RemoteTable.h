#pragma once

#include <BasicView/AbstractStdTable.h>
#include "MagicMenu.h"
#include "JsonRpcClient.h"
#include "TableRpcData.h"
#include "OverlayFrame.h"

#include <QWebSocket>
#include <QWidget>
#include <QTimer>
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
    void wheelEvent(QWheelEvent* event) override;

private:
    enum ColumnIndex
    {
        ColAddress,
        ColData,
    };

    using Epoch = std::chrono::time_point<std::chrono::system_clock>;

    duint mRemoteTableOffset = 0;
    std::vector<std::vector<std::string>> mRemoteData;
    Epoch mLastPrepare;

    QWebSocket mSocket;
    JsonRpcClient mRpc;

    TableRequest mCurrentRequest;
    bool mCurrentSent = false;
    Epoch mCurrentSentTime;

    TableRequest mNextRequest;
    bool mNextRequired = false;

    OverlayFrame* mOverlay = nullptr;

    std::deque<uint64_t> mResponseTimes;
    uint64_t mMinResponseTime = 0;
    uint64_t mMaxResponseTime = 100;
    uint64_t mAvgResponseTime = 100;
    uint64_t mMedResponseTime = 100;

    QTimer* mScrollTimer = nullptr;
    dsint mScrollDelta = 0;
    Epoch mScrollStart;
    std::deque<std::pair<Epoch, dsint>> mScrollEvents;

    void handleTableResponse(const TableResponse & response);
    void setupMenu();
    void setupConnection();
};
