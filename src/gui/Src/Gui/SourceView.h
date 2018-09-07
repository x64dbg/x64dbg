#ifndef SOURCEVIEW_H
#define SOURCEVIEW_H

#include <QWidget>
#include <AbstractStdTable.h>

class FileLines;

class SourceView : public AbstractStdTable
{
    Q_OBJECT
public:
    SourceView(QString path, duint addr, QWidget* parent = nullptr);
    ~SourceView();

    QString getCellContent(int r, int c) override;
    bool isValidIndex(int r, int c) override;
    void sortRows(int column, bool ascending) override;
    void prepareData() override;

    QString getSourcePath();
    void setSelection(duint addr);
    void clear();

private slots:
    void contextMenuSlot(const QPoint & pos);
    void followDisassemblerSlot();
    void gotoLineSlot();
    void openSourceFileSlot();
    void showInDirectorySlot();

private:
    MenuBuilder* mMenuBuilder = nullptr;
    QString mSourcePath;
    duint mModBase;

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
