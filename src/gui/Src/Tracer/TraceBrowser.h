#ifndef TRACEBROWSER_H
#define TRACEBROWSER_H

#include "AbstractTableView.h"
#include "VaHistory.h"
#include "QBeaEngine.h"

class TraceFileReader;
class BreakpointMenu;
class MRUList;

class TraceBrowser : public AbstractTableView
{
    Q_OBJECT
public:
    explicit TraceBrowser(QWidget* parent = 0);
    ~TraceBrowser();

    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

    void prepareData();
    virtual void updateColors();

    void expandSelectionUpTo(duint to);
    void setSingleSelection(duint index);
    duint getInitialSelection();
    duint getSelectionSize();
    duint getSelectionStart();
    duint getSelectionEnd();

private:
    void setupRightClickContextMenu();
    void makeVisible(duint index);
    QString getAddrText(dsint cur_addr, char label[MAX_LABEL_SIZE], bool getLabel);
    QString getIndexText(duint index);
    void pushSelectionInto(bool copyBytes, QTextStream & stream, QTextStream* htmlStream = nullptr);
    void copySelectionSlot(bool copyBytes);
    void copySelectionToFileSlot(bool copyBytes);

    void contextMenuEvent(QContextMenuEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void keyPressEvent(QKeyEvent* event);

    VaHistory mHistory;
    MenuBuilder* mMenuBuilder;
    bool mRvaDisplayEnabled;
    duint mRvaDisplayBase;

    typedef struct _SelectionData_t
    {
        duint firstSelectedIndex;
        duint fromIndex;
        duint toIndex;
    } SelectionData_t;

    SelectionData_t mSelection;
    CapstoneTokenizer::SingleToken mHighlightToken;
    bool mHighlightingMode;
    bool mPermanentHighlightingMode;

    TraceFileReader* mTraceFile;
    QBeaEngine* mDisasm;
    BreakpointMenu* mBreakpointMenu;
    MRUList* mMRUList;
    QString mFileName;

    QColor mBytesColor;
    QColor mBytesBackgroundColor;

    QColor mInstructionHighlightColor;
    QColor mSelectionColor;

    QColor mCipBackgroundColor;
    QColor mCipColor;

    QColor mBreakpointBackgroundColor;
    QColor mBreakpointColor;

    QColor mHardwareBreakpointBackgroundColor;
    QColor mHardwareBreakpointColor;

    QColor mBookmarkBackgroundColor;
    QColor mBookmarkColor;

    QColor mLabelColor;
    QColor mLabelBackgroundColor;

    QColor mSelectedAddressBackgroundColor;
    QColor mTracedAddressBackgroundColor;
    QColor mSelectedAddressColor;
    QColor mAddressBackgroundColor;
    QColor mAddressColor;

    QColor mAutoCommentColor;
    QColor mAutoCommentBackgroundColor;
    QColor mCommentColor;
    QColor mCommentBackgroundColor;

signals:
    void displayReferencesWidget();

public slots:

    void openFileSlot();
    void openSlot(const QString & fileName);
    void toggleRunTraceSlot();
    void closeFileSlot();
    void parseFinishedSlot();
    void tokenizerConfigUpdatedSlot();

    void gotoSlot();
    void gotoPreviousSlot();
    void gotoNextSlot();
    void followDisassemblySlot();
    void enableHighlightingModeSlot();
    void setLabelSlot();
    void setCommentSlot();
    void copyDisassemblySlot();
    void copyCipSlot();
    void copyIndexSlot();
    void copySelectionSlot();
    void copySelectionNoBytesSlot();
    void copySelectionToFileSlot();
    void copySelectionToFileNoBytesSlot();
    void copyFileOffsetSlot();
    void copyRvaSlot();

    void searchConstantSlot();
    void searchMemRefSlot();

    void updateSlot(); //debug
};

#endif //TRACEBROWSER_H
