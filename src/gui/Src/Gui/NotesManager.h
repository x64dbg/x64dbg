#ifndef NOTESMANAGER_H
#define NOTESMANAGER_H

#include <QWidget>
#include <QTabWidget>
#include "NotepadView.h"

class NotesManager : public QTabWidget
{
    Q_OBJECT
public:
    explicit NotesManager(QWidget* parent = 0);

private:
    NotepadView* mGlobal;
    NotepadView* mDebuggee;
};

#endif // NOTESMANAGER_H
