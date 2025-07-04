#include "RichTextItemDelegate.h"

#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QApplication>

RichTextItemDelegate::RichTextItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void RichTextItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem & inOption, const QModelIndex & index) const
{
    QStyleOptionViewItem option = inOption;
    initStyleOption(&option, index);

    if(option.text.isEmpty())
    {
        // This is nothing this function is supposed to handle
        QStyledItemDelegate::paint(painter, inOption, index);

        return;
    }

    QStyle* style = option.widget ? option.widget->style() : QApplication::style();

    QTextOption textOption;
    textOption.setWrapMode(option.features & QStyleOptionViewItem::WrapText ? QTextOption::WordWrap
                           : QTextOption::ManualWrap);
    textOption.setTextDirection(option.direction);

    QTextDocument doc;
    doc.setDefaultTextOption(textOption);
    QString textColor;
    if(option.state & QStyle::State_Selected)
    {
        textColor = option.palette.highlightedText().color().name();
    }
    else
    {
        textColor = option.palette.text().color().name();
    }
    doc.setHtml(QString("<font color=\"%1\">%2</font>").arg(textColor, option.text));
    doc.setDefaultFont(option.font);
    doc.setDocumentMargin(0);
    doc.setTextWidth(option.rect.width());
    doc.adjustSize();

    if(doc.size().width() > option.rect.width())
    {
        // Elide text
        QTextCursor cursor(&doc);
        cursor.movePosition(QTextCursor::End);

        const QString elidedPostfix = "...";
        QFontMetrics metric(option.font);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        int postfixWidth = metric.horizontalAdvance(elidedPostfix);
#else
        int postfixWidth = metric.width(elidedPostfix);
#endif
        while(!doc.isEmpty() && doc.size().width() > option.rect.width() - postfixWidth)
        {
            cursor.deletePreviousChar();
            doc.adjustSize();
        }

        cursor.insertText(elidedPostfix);
    }

    // Painting item without text (this takes care of painting e.g. the highlighted for selected
    // or hovered over items in an ItemView)
    option.text = QString();
    style->drawControl(QStyle::CE_ItemViewItem, &option, painter, inOption.widget);

    // Figure out where to render the text in order to follow the requested alignment
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    QSize documentSize(doc.size().width(), doc.size().height()); // Convert QSizeF to QSize
    QRect layoutRect = QStyle::alignedRect(Qt::LayoutDirectionAuto, option.displayAlignment, documentSize, textRect);

    painter->save();

    // Translate the painter to the origin of the layout rectangle in order for the text to be
    // rendered at the correct position
    painter->translate(layoutRect.topLeft());
    doc.drawContents(painter, textRect.translated(-textRect.topLeft()));

    painter->restore();
}

QSize RichTextItemDelegate::sizeHint(const QStyleOptionViewItem & inOption, const QModelIndex & index) const
{
    QStyleOptionViewItem option = inOption;
    initStyleOption(&option, index);

    if(option.text.isEmpty())
    {
        // This is nothing this function is supposed to handle
        return QStyledItemDelegate::sizeHint(inOption, index);
    }

    QTextDocument doc;
    doc.setHtml(option.text);
    doc.setTextWidth(option.rect.width());
    doc.setDefaultFont(option.font);
    doc.setDocumentMargin(0);

    return QSize(doc.idealWidth(), doc.size().height());
}
