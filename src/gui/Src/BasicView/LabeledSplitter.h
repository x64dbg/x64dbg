#ifndef LABELEDSPLITTER_H
#define LABELEDSPLITTER_H

#include <QSplitter>

class LabeledSplitter;
class QMenu;
class QAction;

class LabeledSplitterHandle : public QSplitterHandle
{
    Q_OBJECT
public:
    LabeledSplitterHandle(Qt::Orientation o, LabeledSplitter* parent);
protected slots:
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void detachSlot();
    void collapseSlot();
protected:
    int getIndex();
    int charHeight;
    int originalSize;
    int originalIndex;
    void setupContextMenu();
    LabeledSplitter* getParent() const;
    QMenu* mMenu;
    QAction* mExpandCollapseAction;
};

class LabeledSplitter : public QSplitter
{
    Q_OBJECT
public:
    LabeledSplitter(QWidget* parent);
    void addWidget(QWidget* widget, const QString & name);
    void insertWidget(int index, QWidget* widget, const QString & name);
    void collapseLowerTabs();
    QList<QString> names;
    QList<QWidget*> m_Windows;
public slots:
    void attachSlot(QWidget* widget);
protected:
    void setOrientation(Qt::Orientation o); // LabeledSplitter is always vertical
    void addWidget(QWidget* widget);
    void insertWidget(int index, QWidget* widget);
    QSplitterHandle* createHandle() override;

    friend class LabeledSplitterHandle;
};

#endif //LABELEDSPLITTER_H
