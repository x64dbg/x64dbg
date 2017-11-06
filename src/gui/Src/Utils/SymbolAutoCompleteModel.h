#ifndef SYMBOLAUTOCOMPLETEMODEL
#define SYMBOLAUTOCOMPLETEMODEL
#include <functional>
#include <QAbstractItemModel>
#define MAXAUTOCOMPLETEENTRY 20

class QRegularExpression;

class SymbolAutoCompleteModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    SymbolAutoCompleteModel(std::function<QString()> getTextProc, QObject* parent = 0);
    ~SymbolAutoCompleteModel();

    virtual QVariant data(const QModelIndex & index, int role) const override;
    virtual QModelIndex index(int row, int column, const QModelIndex & parent) const override;
    virtual int rowCount(const QModelIndex & parent) const override;
    virtual int columnCount(const QModelIndex & parent) const override;
    virtual QModelIndex parent(const QModelIndex & child) const override;
private:
    std::function<QString()> mGetTextProc;
    QRegularExpression* isValidReg;
};
#endif //SYMBOLAUTOCOMPLETEMODEL
