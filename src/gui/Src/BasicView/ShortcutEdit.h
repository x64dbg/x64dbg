#ifndef SHORTCUTEDIT_H
#define SHORTCUTEDIT_H

#include <QLineEdit>
#include <QKeySequence>
#include <QKeyEvent>

class ShortcutEdit : public QLineEdit
{
    Q_OBJECT
    int keyInt;

public:
    explicit ShortcutEdit(QWidget* parent = 0);
    const QKeySequence getKeysequence() const;

public slots:
    void setErrorState(bool error);

signals:
    void askForSave();

protected:
    void keyPressEvent(QKeyEvent* event);
};

#endif // SHORTCUTEDIT_H
