#include "RemoteTable.h"
#include "StringUtil.h"
#include "Bridge.h"

#include <QMessageBox>
#include <QTimer>

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
}

QString RemoteTable::getCellContent(duint row, duint col)
{
    auto relativeRow = row - getTableOffset();
    auto base = QString("row: %1, col: %2").arg(row, 5).arg(col);
    if(relativeRow < mRemoteData.size())
    {
        auto data = QString::fromStdString(mRemoteData[relativeRow][col]);
        return base + " " + data;
    }
    else
    {
        return "(FETCHING DATA...)";
        return QString();
        return base + " (FETCHING DATA...)";
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

    // TODO: do some cooldown and don't enqueue unnecessary requests
    // TODO: use the average/median/last latency of the connection as a debouncing timer
    // TODO: use some basic prediction heuristics (scroll direction etc) to anticipate the scroll
    // TODO: measure the ping

    TableRequest r;
    r.offset = offset;
    r.lines = linesToPrint;
    r.scroll = 0; // TODO

    if(!mCurrentSent)
    {
        assert(!mNextRequired);
        mCurrentSent = true;

        qDebug() << "send rpc request";
        mCurrentRequest = r;
        mRpc.call<TableResponse>(r, [this](TableResponse response) { handleTableResponse(response); });
    }
    else
    {
        qDebug() << "add rpc to backlog";
        mNextRequest = r;
        mNextRequired = true;
    }
}

void RemoteTable::handleTableResponse(const TableResponse & response)
{
    if(mNextRequired)
    {
        mNextRequired = false;
        mCurrentRequest = mNextRequest;
        assert(mCurrentSent);

        qDebug() << "send rpc request (backlog)";

        mRpc.call<TableResponse>(mCurrentRequest, [this](TableResponse response) { handleTableResponse(response); });
    }
    else
    {
        mCurrentSent = false;
        qDebug() << "final response";
    }

    mRemoteData = std::move(response.rows);
    updateViewport();
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
