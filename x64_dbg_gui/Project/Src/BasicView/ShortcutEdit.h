#ifndef SHORTCUTEDIT_H
#define SHORTCUTEDIT_H

#include <QLineEdit>

class ShortcutEdit : public QLineEdit
{
    Q_OBJECT
    QKeySequence key;
    int keyInt;
public:
    explicit ShortcutEdit(QWidget *parent = 0);

    const QKeySequence getKeysequence() const;

public slots:
    void setErrorState(bool error);
signals:
    void askForSave();
protected:
    void keyPressEvent ( QKeyEvent * event );

};

#endif // SHORTCUTEDIT_H
