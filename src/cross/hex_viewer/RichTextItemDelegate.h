#pragma once

#include <QStyledItemDelegate>

// Based on: https://stackoverflow.com/a/66412883/1806760
class RichTextItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit RichTextItemDelegate(QObject* parent = nullptr);

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;
};

