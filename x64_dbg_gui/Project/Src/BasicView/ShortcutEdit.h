#ifndef SHORTCUTEDIT_H
#define SHORTCUTEDIT_H

#include <QLineEdit>

class ShortcutEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ShortcutEdit(QWidget *parent = 0);

signals:

protected:
    void keyPressEvent ( QKeyEvent * event );

};

#endif // SHORTCUTEDIT_H
