#ifndef SOURCEVIEW_H
#define SOURCEVIEW_H

#include <QWidget>
#include <AbstractStdTable.h>
#include "BreakpointMenu.h"

class FileLines;

class SourceView : public AbstractStdTable
{
    Q_OBJECT
public:
    SourceView(QString path, duint addr, QWidget* parent = nullptr);
    ~SourceView();

    QString getCellContentUnsafe(int r, int c) override;
    bool isValidIndex(int r, int c) override;
    void sortRows(int column, bool ascending) override;
    void prepareData() override;

    QString getSourcePath();
    void setSelection(duint addr);
    void clear();

private slots:
    void contextMenuSlot(const QPoint & pos);
    void followDisassemblerSlot();
    void followDumpSlot();
    void toggleBookmarkSlot();
    void gotoLineSlot();
    void openSourceFileSlot();
    void showInDirectorySlot();

private:
    MenuBuilder* mMenuBuilder = nullptr;
    BreakpointMenu* mBreakpointMenu = nullptr;
    QString mSourcePath;
    duint mModBase;
    int mTabSize = 4; //TODO: make customizable?

    FileLines* mFileLines = nullptr;

    enum
    {
        ColAddr,
        ColLine,
        ColCode,
    };

    struct CodeData
    {
        QString code;
    };

    struct LineData
    {
        duint addr;
        size_t index;
        CodeData code;
    };

    dsint mPrepareTableOffset = 0;
    std::vector<LineData> mLines;

    void setupContextMenu();
    void loadFile();
    void parseLine(size_t index, LineData & line);
    duint addrFromIndex(size_t index);
};

#endif // SOURCEVIEW_H
