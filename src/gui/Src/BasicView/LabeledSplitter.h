#ifndef LABELEDSPLITTER_H
#define LABELEDSPLITTER_H

#include <QSplitter>

class LabeledSplitter;

class LabeledSplitterHandle : public QSplitterHandle
{
    Q_OBJECT
public:
    LabeledSplitterHandle(Qt::Orientation o, LabeledSplitter* parent);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
protected:
    int getIndex();
    int charHeight;
    int originalSize;
    LabeledSplitter* getParent() const;
};

class LabeledSplitter : public QSplitter
{
    Q_OBJECT
public:
    LabeledSplitter(QWidget* parent);
    void addWidget(QWidget* widget, const QString & name);
    void insertWidget(int index, QWidget* widget, const QString & name);
    QString getName(int index) const;
protected:
    void setOrientation(Qt::Orientation o); // LabeledSplitter is always vertical
    void addWidget(QWidget* widget);
    void insertWidget(int index, QWidget* widget);
    QSplitterHandle* createHandle() override;
    QList<QString> names;
};

#endif //LABELEDSPLITTER_H
