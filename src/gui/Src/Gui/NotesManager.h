#ifndef NOTESMANAGER_H
#define NOTESMANAGER_H

#include <QWidget>
#include <QTabWidget>
#include "NotepadView.h"
#include "Bridge.h"

class NotesManager : public QTabWidget
{
    Q_OBJECT
public:
    explicit NotesManager(QWidget* parent = 0);

public slots:
    void dbgStateChangedSlot(DBGSTATE state);

private:
    NotepadView* mGlobal;
    NotepadView* mDebuggee;
};

#endif // NOTESMANAGER_H
