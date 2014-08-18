#ifndef LABELCLASS_H
#define LABELCLASS_H

class StatusLabel : public QLabel
{
    Q_OBJECT
public:
    explicit StatusLabel(QStatusBar* parent = 0);

public slots:
    void debugStateChangedSlot(DBGSTATE state);
    void logUpdate(QString message);

private:
    QString labelText;

};

#endif // LABELCLASS_H
