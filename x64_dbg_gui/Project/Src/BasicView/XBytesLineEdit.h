#ifndef XBYTESLINEEDIT_H
#define XBYTESLINEEDIT_H

#include <QLineEdit>

// non-fixed bytesedit (QLineEdit with auto-grow InputMask)
class XBytesLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit XBytesLineEdit(QWidget *parent = 0);

    void copy();
    void cut();
    QString text();
signals:

public slots:
    void paste();
    void keyPressEvent(QKeyEvent *event);
protected:

protected slots:
    void autoMask(QString content);
    void modSelection();

private:
    int mParts;
};

#endif // XBYTEEDIT_H
