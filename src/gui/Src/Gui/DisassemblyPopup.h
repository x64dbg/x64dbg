#ifndef DISASSEMBLYPOPUP_H
#define DISASSEMBLYPOPUP_H
#include <QFrame>
#include "Imports.h"
#include "RichTextPainter.h"
#include "CachedFontMetrics.h"
#include "QBeaEngine.h"

class Disassembly;

class DisassemblyPopup : public QFrame
{
    Q_OBJECT
public:
    explicit DisassemblyPopup(Disassembly* parent);
    ~DisassemblyPopup();
    void paintEvent(QPaintEvent* event);
    void setAddress(duint Address);
    duint getAddress();
public slots:
    void hide();
    void updateFont();
    void updateColors();
protected:
    Disassembly* parent;
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
    QColor labelColor;
    QColor labelBackgroundColor;
    QColor commentColor;
    QColor commentBackgroundColor;
    QColor commentAutoColor;
    QColor commentAutoBackgroundColor;

    QList<Instruction_t> mInstBuffer;
    std::vector<RichTextPainter::List> mDisassemblyToken;
};

#endif // DISASSEMBLYPOPUP_H
