#pragma once

#include <QMainWindow>
#include <QSplitterHandle>

class LabeledSplitter;

class LabeledSplitterHandle : public QSplitterHandle
{
    Q_OBJECT
public:
    LabeledSplitterHandle(Qt::Orientation o, LabeledSplitter* parent);
    int originalSize;

protected slots:
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

protected:
    int getIndex();
    LabeledSplitter* getParent() const;

    int charHeight;
};

class LabeledSplitterDetachedWindow : public QMainWindow
{
    Q_OBJECT

public:
    LabeledSplitterDetachedWindow(QWidget* parent = 0, LabeledSplitter* splitterwidget = 0);
    ~LabeledSplitterDetachedWindow();

    int index;

signals:
    void OnClose(LabeledSplitterDetachedWindow* widget);

protected:
    void closeEvent(QCloseEvent* event);

    LabeledSplitter* mSplitterWidget;
};
