#ifndef LABELEDSPLITTERDETACHEDWINDOW_H
#define LABELEDSPLITTERDETACHEDWINDOW_H

#include <QMainWindow>

class LabeledSplitter;

class LabeledSplitterDetachedWindow : public QMainWindow
{
    Q_OBJECT

public:
    LabeledSplitterDetachedWindow(QWidget* parent = 0, LabeledSplitter* splitterwidget = 0);
    ~LabeledSplitterDetachedWindow(void);
    int index;

protected:
    LabeledSplitter* m_SplitterWidget;

    void closeEvent(QCloseEvent* event);

signals:
    void OnClose(QWidget* widget);
};


#endif //LABELEDSPLITTERDETACHEDWINDOW_H
