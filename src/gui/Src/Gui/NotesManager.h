#ifndef NOTESMANAGER_H
#define NOTESMANAGER_H

#include <QWidget>
#include <QTabWidget>
#include "NotepadView.h"

class NotesManager : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY(int viewId MEMBER m_viewId)
public:
    explicit NotesManager(QWidget* parent = 0);

private:
    int m_viewId;
    NotepadView* mGlobal;
    NotepadView* mDebuggee;
};

#endif // NOTESMANAGER_H
