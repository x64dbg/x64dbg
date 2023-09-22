#include "SymbolAutoCompleteModel.h"
#include "MiscUtil.h"
#include <Configuration.h>

SymbolAutoCompleteModel::SymbolAutoCompleteModel(std::function<QString()> getTextProc, QObject* parent)
    : QAbstractItemModel(parent),
      mGetTextProc(getTextProc),
      isValidReg("[\\w_@][\\w\\d_]*")
{
    lastAutocompleteCount = 0;
    disableAutoCompleteUpdated();
    connect(Config(), SIGNAL(disableAutoCompleteUpdated()), this, SLOT(disableAutoCompleteUpdated()));
}

QModelIndex SymbolAutoCompleteModel::parent(const QModelIndex & child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

QModelIndex SymbolAutoCompleteModel::index(int row, int column, const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

int SymbolAutoCompleteModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    if(DbgIsDebugging())
    {
        QString text = mGetTextProc();
        auto match = isValidReg.match(text);
        if(match.hasMatch())
        {
            update();
            return lastAutocompleteCount;
        }
        else
            return 0;
    }
    else
        return 0;
}

int SymbolAutoCompleteModel::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant SymbolAutoCompleteModel::data(const QModelIndex & index, int role) const
{
    if(index.isValid() && DbgIsDebugging())
    {
        if(index.column() == 0)
        {
            if(role == Qt::DisplayRole || role == Qt::EditRole || role == Qt::AccessibleTextRole)
            {
                //TODO
                update();
                if(index.row() < lastAutocompleteCount)
                    return QVariant(lastAutocomplete[index.row()]);
                else
                    return QVariant();
            }
            else if(role == Qt::DecorationRole)
            {
                return QVariant(DIcon("functions"));
            }
        }
    }
    return QVariant();
}

void SymbolAutoCompleteModel::update() const
{
    QString text = mGetTextProc();
    if(text == lastAutocompleteText)
        return;
    char* data[MAXAUTOCOMPLETEENTRY];
    memset(data, 0, sizeof(data));
    int count = DbgFunctions()->SymAutoComplete(text.toUtf8().constData(), (char**)data, MAXAUTOCOMPLETEENTRY);
    for(int i = 0; i < count; i++)
    {
        lastAutocomplete[i] = QString::fromUtf8(data[i]);
        BridgeFree(data[i]);
    }
    lastAutocompleteCount = count;
    lastAutocompleteText = text;
}

void SymbolAutoCompleteModel::disableAutoCompleteUpdated()
{
    lastAutocompleteText = "";
}
