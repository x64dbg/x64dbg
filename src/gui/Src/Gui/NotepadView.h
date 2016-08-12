#ifndef NOTEPADVIEW_H
#define NOTEPADVIEW_H

#include <QPlainTextEdit>

class NotepadView : public QPlainTextEdit
{
    Q_OBJECT
    Q_PROPERTY(int viewId MEMBER m_viewId)
public:
    explicit NotepadView(QWidget* parent = 0);
	
public slots:
    void updateStyle();
    void setNotes(const QString text);
    void getNotes(void* ptr);

private:
    int m_viewId;
};

#endif // NOTEPADVIEW_H
