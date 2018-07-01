#ifndef SOURCEVIEW_H
#define SOURCEVIEW_H

#include <QWidget>
#include <QMenu>
#include <QAction>
#include "StdTable.h"
#include "ReferenceView.h"

class SourceView : public ReferenceView
{
    Q_OBJECT
public:
    explicit SourceView(QString path, duint addr, QWidget* parent = 0);
    QString getSourcePath();
    void setupContextMenu();
    void setSelection(duint addr);

private slots:
    void sourceContextMenu(QMenu* menu);
    void openSourceFileSlot();
    void showInDirectorySlot();

private:
    QString mSourcePath;
    int mIpLine;
    MenuBuilder* mMenuBuilder;
    duint mModBase;

    void loadFile();
};

#endif // SOURCEVIEW_H
