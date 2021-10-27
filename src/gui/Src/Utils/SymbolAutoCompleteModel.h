#pragma once

#include <functional>
#include <QAbstractItemModel>
#include <QRegularExpression>
#define MAXAUTOCOMPLETEENTRY 20

class SymbolAutoCompleteModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    SymbolAutoCompleteModel(std::function<QString()> getTextProc, QObject* parent = 0);

    virtual QVariant data(const QModelIndex & index, int role) const override;
    virtual QModelIndex index(int row, int column, const QModelIndex & parent) const override;
    virtual int rowCount(const QModelIndex & parent) const override;
    virtual int columnCount(const QModelIndex & parent) const override;
    virtual QModelIndex parent(const QModelIndex & child) const override;

private slots:
    void disableAutoCompleteUpdated();

private:
    std::function<QString()> mGetTextProc;
    QRegularExpression isValidReg;

    mutable QString lastAutocompleteText;
    mutable QString lastAutocomplete[MAXAUTOCOMPLETEENTRY];
    mutable int lastAutocompleteCount;
    void update() const;
};
