#include "RemoteTable.h"
#include "StringUtil.h"
#include "Bridge.h"

#include <QMessageBox>
#include <QTimer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QProgressBar>
#include <QLabel>
#include <QTextBrowser>

RemoteTable::RemoteTable(QWidget* parent)
    : AbstractStdTable(parent)
{
    std::map<ColumnIndex, std::pair<int, QString>> columns;
    columns[ColAddress] = { 50, tr("Address") };
    columns[ColData] = { 50, tr("Data") };

    int col = 0;
    for(const auto & itr : columns)
    {
        if(itr.first != col)
        {
            qDebug() << "Bad column index" << col << "!=" << itr.first;
            QApplication::exit(1);
        }
        addColumnAt(calculateColumnWidth(itr.second.first), itr.second.second, true);
        col++;
    }

    setupMenu();

    mRpc.open(QUrl("ws://127.0.0.1:42069"));
    connect(&mRpc, &JsonRpcClient::connected, this, [this]()
    {
        prepareData();
    });

    // Test
    setRowCount(0x2000ull);

    mOverlay = OverlayFrame::embed(this, false);
}

QString RemoteTable::getCellContent(duint row, duint col)
{
    // TODO: get data from a container that's aware of the outgoing requests
    // (perhaps this should be done transparently in the request/response handling?)
    auto relativeRow = row - getTableOffset();
    QString base;
    if(col == 0)
    {
        base = QString("[row: %1] ").arg(row, 5);
    }
    if(relativeRow < mRemoteData.size())
    {
        const auto & remoteRow = mRemoteData[relativeRow];
        QString data;
        if(col < remoteRow.size())
        {
            data = QString::fromStdString(remoteRow[col]);
        }
        else
        {
            data = "(BAD SERVER)";
        }
        return base + data;
    }
    else
    {
        return base + "(FETCHING DATA...)";
    }
}

duint RemoteTable::getCellUserdata(duint row, duint column)
{
    return 0;
}

bool RemoteTable::isValidIndex(duint row, duint col)
{
    if(row >= getRowCount())
        return false;

    return col >= ColAddress && col <= ColData;
}

void RemoteTable::sortRows(duint column, bool ascending)
{
}

duint RemoteTable::sliderMovedHook(QScrollBar::SliderAction action, duint value, dsint delta)
{
    auto actionName = [action]()
    {
        switch(action)
        {
        case QScrollBar::SliderNoAction:
            return "SliderNoAction";
        case QScrollBar::SliderSingleStepAdd:
            return "SliderSingleStepAdd";
        case QScrollBar::SliderSingleStepSub:
            return "SliderSingleStepSub";
        case QScrollBar::SliderPageStepAdd:
            return "SliderPageStepAdd";
        case QScrollBar::SliderPageStepSub:
            return "SliderPageStepSub";
        case QScrollBar::SliderToMinimum:
            return "SliderToMinimum";
        case QScrollBar::SliderToMaximum:
            return "SliderToMaximum";
        case QScrollBar::SliderMove:
            return "SliderMove";
        default:
            return "<unknown>";
        }
    }();
    //qDebug() << "sliderMovedHook" << actionName << "value" << value << "delta" << delta;
    return AbstractStdTable::sliderMovedHook(action, value, delta);
}

void RemoteTable::prepareData()
{
    AbstractStdTable::prepareData();

    if(!mRpc.isConnected())
    {
        qDebug() << "socket not ready bro";
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastPrepare).count();
    mLastPrepare = now;

    auto linesToPrint = getNbrOfLineToPrint();
    auto offset = getTableOffset();

    //qDebug() << "offset" << offset << "lines" << linesToPrint << "last" << offset + linesToPrint - 1 << "elapsed" << elapsedMs;

#if 0
    const uint32_t totalRefreshMs = 300;
    const uint32_t scrollDebounceMs = 300;

    if(elapsedMs >= totalRefreshMs)
    {
        qDebug() << "100% send request";
    }
    else if(elapsedMs < scrollDebounceMs)
    {
        qDebug() << "scroll debounce";
    }
    else
    {
        qDebug() << "unclear request";
    }
#endif

    //mRemoteData.clear();
    //qDebug() << "prepareData";

    // TODO: do some cooldown and don't enqueue unnecessary requests (while dragging the scroll bar)
    // TODO: use the average/median/last latency of the connection as a debouncing timer
    // TODO: use some basic prediction heuristics (scroll direction etc) to anticipate
    // the scroll (request a bigger range)
    // TODO: measure the ping
    // TODO: implement partial reuse of the data

    TableRequest r;
    r.offset = offset;
    r.lines = linesToPrint;
    r.scroll = 0; // TODO: unused for now, used to scroll up in disassembly (variable size)

    if(!mCurrentSent)
    {
        assert(!mNextRequired);
        mCurrentSent = true;
        mCurrentSentTime = now;
        mOverlay->setVisible(true);

        qDebug() << "send rpc request";
        mCurrentRequest = r;
        mRpc.call<TableResponse>(r, [this](TableResponse response) { handleTableResponse(response); });
    }
    else
    {
        //qDebug() << "add rpc to backlog";
        mNextRequest = r;
        mNextRequired = true;
    }
}

void RemoteTable::handleTableResponse(const TableResponse & response)
{
    // Calculate response time statistics
    auto now = std::chrono::system_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - mCurrentSentTime).count();
    mResponseTimes.push_back(elapsedMs);
    const size_t MAX_RESPONSE_TIMES = 10;
    while(mResponseTimes.size() > MAX_RESPONSE_TIMES)
    {
        mResponseTimes.pop_front();
    }
    std::vector<uint64_t> sorted;
    sorted.reserve(mResponseTimes.size());
    mMaxResponseTime = 0;
    mMinResponseTime = UINT64_MAX;
    mAvgResponseTime = 0;
    for(const auto & responseTime : mResponseTimes)
    {
        mMaxResponseTime = std::max(mMaxResponseTime, responseTime);
        mMinResponseTime = std::min(mMinResponseTime, responseTime);
        mAvgResponseTime += responseTime;
        sorted.push_back(responseTime);
    }
    mAvgResponseTime /= mResponseTimes.size();
    std::sort(sorted.begin(), sorted.end());
    auto middle = sorted.size() / 2;
    if(sorted.size() % 2 == 0)
    {
        mMedResponseTime = (sorted[middle - 1] + sorted[middle]) / 2;
    }
    else
    {
        mMedResponseTime = sorted[middle];
    }

    qDebug() << "[response time] now:" << elapsedMs << "min:" << mMinResponseTime << "max:" << mMaxResponseTime << "avg:" << mAvgResponseTime << "med:" << mMedResponseTime;

    // Handle deferred request
    if(mNextRequired)
    {
        mNextRequired = false;
        auto previousRequest = mCurrentRequest;
        mCurrentRequest = mNextRequest;
        assert(mCurrentSent);
        mCurrentSentTime = now;

        qDebug() << "send rpc request (backlog)";

        mRpc.call<TableResponse>(mCurrentRequest, [this](TableResponse response) { handleTableResponse(response); });

        // TODO: display the results that fit the new outgoing request?
        auto tableOffset = getTableOffset();
        qDebug() << "[partial] tableOffset:" << tableOffset << "requestOffset:" << previousRequest.offset << "@" << previousRequest.lines;
        // The first row displayed here depends on the difference between the current table offset and the previous offset requested
        if(tableOffset < previousRequest.offset)
        {
            // TODO: ??
            qDebug() << "TODO: implement negative offset";
        }
        else
        {
            auto diff = tableOffset - previousRequest.offset;
            for(int i = 0; i < mRemoteData.size(); i++)
            {

            }
        }
    }
    else
    {
        mCurrentSent = false;
        qDebug() << "final response";

        mOverlay->setVisible(false);

        // Update the displayed data with the final response
        mRemoteData = std::move(response.rows);
        updateViewport();
    }
}

void RemoteTable::setupMenu()
{
    addMenuAction(TableAction(TR("Goto line"), "G"), [this]()
    {
        qDebug() << "scroll select";
        auto ok = scrollSelect(5000);
        if(!ok)
            qDebug() << "failz";
        reloadData();
    });
}
