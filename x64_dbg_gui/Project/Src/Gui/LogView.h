#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QTextEdit>

class LogView : public QTextEdit
{
    Q_OBJECT
public:
    explicit LogView(QWidget* parent = 0);

signals:

public slots:
    void updateStyle();
    void addMsgToLogSlot(QString msg);
    void clearLogSlot();

private:



};

#endif // LOGVIEW_H
