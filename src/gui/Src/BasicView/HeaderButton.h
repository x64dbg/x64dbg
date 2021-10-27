#pragma once

#include <QWidget>

class HeaderButton : public QWidget
{
    Q_OBJECT
public:
    explicit HeaderButton(QWidget* parent = 0);
    void setGeometry(int x, int y, int w, int h);

signals:

public slots:

};
