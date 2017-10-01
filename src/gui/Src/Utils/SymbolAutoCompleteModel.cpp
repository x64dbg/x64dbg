#include "SymbolAutoCompleteModel.h"
#include "MiscUtil.h"

static QString Decorate(const QString & text)
{
    return "*!" + text + "*";
}

SymbolAutoCompleteModel::SymbolAutoCompleteModel(std::function<QString()> getTextProc, QObject* parent) : QAbstractItemModel(parent), mGetTextProc(getTextProc)
{

}

QModelIndex SymbolAutoCompleteModel::parent(const QModelIndex & child) const
{
    return QModelIndex();
}

QModelIndex SymbolAutoCompleteModel::index(int row, int column, const QModelIndex & parent) const
{
    return createIndex(row, column);
}

int SymbolAutoCompleteModel::rowCount(const QModelIndex & parent) const
{
    if(DbgIsDebugging())
        return DbgSymAutoComplete(Decorate(mGetTextProc()).toUtf8().constData(), nullptr);
    else
        return 0;
}

int SymbolAutoCompleteModel::columnCount(const QModelIndex & parent) const
{
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
                QVariant value;
                char* data[20];
                memset(data, 0, sizeof(data));
                int count = DbgSymAutoComplete(Decorate(mGetTextProc()).toUtf8().constData(), (char**)data);
                if(index.row() < count)
                    value = QVariant(QString::fromUtf8(data[index.row()]));
                else
                    value = QVariant();
                for(int i = 0; i < count; i++)
                {
                    BridgeFree(data[i]);
                }
                return value;
            }
            else if(role == Qt::DecorationRole)
            {
                return QVariant(DIcon("functions.png"));
            }
        }
    }
    return QVariant();
}
