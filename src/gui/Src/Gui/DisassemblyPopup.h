#ifndef DISASSEMBLYPOPUP_H
#define DISASSEMBLYPOPUP_H

#include <QFrame>
#include "Imports.h"
#include "QBeaEngine.h"

class CachedFontMetrics;

class DisassemblyPopup : public QFrame
{
    Q_OBJECT
public:
    explicit DisassemblyPopup(QWidget* parent);
    void paintEvent(QPaintEvent* event);
    void setAddress(duint Address);
    duint getAddress();

public slots:
    void hide();
    void updateFont();
    void updateColors();
    void tokenizerConfigUpdated();

protected:
    CachedFontMetrics* mFontMetrics;
    duint addr;
    QString addrText;
    QString addrComment;
    bool addrCommentAuto;
    int charWidth;
    int charHeight;
    int mWidth;
    unsigned int mMaxInstructions;

    QColor disassemblyBackgroundColor;
    QColor disassemblyTracedColor;
    QColor labelColor;
    QColor labelBackgroundColor;
    QColor commentColor;
    QColor commentBackgroundColor;
    QColor commentAutoColor;
    QColor commentAutoBackgroundColor;
    QBeaEngine mDisasm;

    std::vector<std::pair<RichTextPainter::List, bool>> mDisassemblyToken;
};

#endif // DISASSEMBLYPOPUP_H
