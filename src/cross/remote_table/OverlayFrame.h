#pragma once

#include <QFrame>
#include <QPushButton>
#include <QLabel>

namespace Ui
{
    class OverlayFrame;
}

class OverlayFrame : public QFrame
{
    Q_OBJECT

public:
    static OverlayFrame* embed(QWidget* parent, bool visible = true);

    explicit OverlayFrame(QWidget* parent = nullptr);
    ~OverlayFrame();

    QPushButton* button();
    QLabel* label();

private:
    Ui::OverlayFrame* ui = nullptr;
};
