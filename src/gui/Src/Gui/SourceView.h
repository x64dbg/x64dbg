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
    Q_PROPERTY(int viewId MEMBER m_viewId)
public:
    explicit SourceView(QString path, int line = 0, StdTable* parent = 0);
    QString getSourcePath();
    void setupContextMenu();
    void setSelection(int line);
	
private:
    int m_viewId;
    QString mSourcePath;
    int mIpLine;
    void loadFile();
};

#endif // SOURCEVIEW_H
