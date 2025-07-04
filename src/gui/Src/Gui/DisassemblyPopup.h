#pragma once

#include <QFrame>
#include <Imports.h>
#include <Disassembler/QZydis.h>
#include <BasicView/AbstractTableView.h>

class CachedFontMetrics;

class DisassemblyPopup : public QFrame
{
    Q_OBJECT
public:
    DisassemblyPopup(AbstractTableView* parent, Architecture* architecture);
    void setAddress(duint Address);
    duint getAddress() const;

public slots:
    void hide();
    void updateFont();
    void updateColors();
    void tokenizerConfigUpdated();

protected:
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* object, QEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    void stopPopupTimer();

    CachedFontMetrics* mFontMetrics = nullptr;
    duint mAddr = 0;
    QString mAddrText;
    QString mAddrComment;
    bool mAddrCommentAuto = false;
    int mCharWidth = 0;
    int mCharHeight = 0;
    int mWidth = 0;
    int mPopupTimer = 0;
    unsigned int mMaxInstructions = 20;

    QColor mDisassemblyBackgroundColor;
    QColor mDisassemblyTracedColor;
    QColor mLabelColor;
    QColor mLabelBackgroundColor;
    QColor mCommentColor;
    QColor mCommentBackgroundColor;
    QColor mCommentAutoColor;
    QColor mCommentAutoBackgroundColor;
    QZydis mDisasm;
    AbstractTableView* mParent = nullptr;

    std::vector<std::pair<RichTextPainter::List, bool>> mDisassemblyToken;
};
