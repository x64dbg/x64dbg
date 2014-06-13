#ifndef DISASSEMBLY_H
#define DISASSEMBLY_H

#include <QtGui>
#include <QtDebug>
#include "NewTypes.h"
#include "Bridge.h"
#include "AbstractTableView.h"
#include "QBeaEngine.h"
#include "RichTextPainter.h"

class Disassembly : public AbstractTableView
{
    Q_OBJECT
public:
    explicit Disassembly(QWidget *parent = 0);
    void colorsUpdated();

    // Reimplemented Functions
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);

    // Mouse Management
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    // Keyboard Management
    void keyPressEvent(QKeyEvent* event);

    // ScrollBar Management
    int_t sliderMovedHook(int type, int_t value, int_t delta);

    // Jumps Graphic
    int paintJumpsGraphic(QPainter* painter, int x, int y, int_t addr);

    // Function Graphic

    enum Function_t
    {
        Function_none,
        Function_single,
        Function_start,
        Function_middle,
        Function_loop_entry,
        Function_end
    };

    int paintFunctionGraphic(QPainter* painter, int x, int y, Function_t funcType, bool loop);

    // Instructions Management
    int_t getPreviousInstructionRVA(int_t rva, uint_t count);
    int_t getNextInstructionRVA(int_t rva, uint_t count);
    int_t getInstructionRVA(int_t index, int_t count);
    Instruction_t DisassembleAt(int_t rva);
    Instruction_t DisassembleAt(int_t rva, int_t count);

    // Selection Management
    void expandSelectionUpTo(int_t to);
    void setSingleSelection(int_t index);
    int_t getInitialSelection();
    int_t getSelectionSize();
    int_t getSelectionStart();
    int_t getSelectionEnd();
    void selectNext();
    void selectPrevious();
    bool isSelected(int_t base, int_t offset);
    bool isSelected(QList<Instruction_t>* buffer, int index);

    // Update/Reload/Refresh/Repaint
    void prepareData();
    void reloadData();

    // Public Methods
    uint_t rvaToVa(int_t rva);
    void disassembleClear();
    const int_t getBase() const;
    int_t getSize();

    // history management
    void historyClear();
    void historyPrevious();
    void historyNext();
    bool historyHasPrevious();
    bool historyHasNext();

    //disassemble
    void disassembleAt(int_t parVA, int_t parCIP, bool history, int_t newTableOffset);

    QList<Instruction_t> *instructionsBuffer();
    const int_t baseAddress() const;
    const int_t currentEIP() const;

signals:
    void selectionChanged(int_t parVA);
    void disassembledAt(int_t parVA, int_t parCIP, bool history, int_t newTableOffset);

public slots:
    void disassembleAt(int_t parVA, int_t parCIP);
    void debugStateChangedSlot(DBGSTATE state);

private:
    enum GuiState_t {NoState, MultiRowsSelectionState};
    enum GraphicDump_t {GD_Nothing, GD_FootToTop, GD_FootToBottom, GD_HeadFromTop, GD_HeadFromBottom, GD_Vert}; // GD_FootToTop = '- , GD_FootToBottom = ,- , GD_HeadFromTop = '-> , GD_HeadFromBottom = ,-> , GD_Vert = |

    typedef struct _SelectionData_t
    {
        int_t firstSelectedIndex;
        int_t fromIndex;
        int_t toIndex;
    } SelectionData_t;

    QBeaEngine* mDisasm;

    SelectionData_t mSelection;

    bool mIsLastInstDisplayed;

    GuiState_t mGuiState;

    int_t mBase;
    int_t mSize;

    int_t mCipRva;

    QList<Instruction_t> mInstBuffer;

    typedef struct _HistoryData_t
    {
        int_t va;
        int_t tableOffset;
    } HistoryData_t;

    QList<HistoryData_t> mVaHistory;
    int mCurrentVa;

protected:
    bool mRvaDisplayEnabled;
    uint_t mRvaDisplayBase;
    int_t mRvaDisplayPageBase;
};

#endif // DISASSEMBLY_H
