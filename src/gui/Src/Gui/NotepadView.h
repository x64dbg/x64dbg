#ifndef NOTEPADVIEW_H
#define NOTEPADVIEW_H

#include <QPlainTextEdit>

class NotepadView : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit NotepadView(QWidget* parent = 0);

public slots:
    void updateStyle();
    void setNotes(const QString text);
    void getNotes(void* ptr);

};

#endif // NOTEPADVIEW_H
