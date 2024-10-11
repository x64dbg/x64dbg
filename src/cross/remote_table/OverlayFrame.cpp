#include "OverlayFrame.h"
#include "ui_OverlayFrame.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

OverlayFrame* OverlayFrame::embed(QWidget* parent, bool visible)
{
    // TODO: confirm the memory management is correct
    auto frame = new OverlayFrame(parent);
    frame->setVisible(visible);
    auto hlayout = new QHBoxLayout(frame);
    hlayout->addWidget(frame);
    auto vlayout = new QVBoxLayout(parent);
    vlayout->addLayout(hlayout);
    return frame;
}

OverlayFrame::OverlayFrame(QWidget* parent) :
    QFrame(parent),
    ui(new Ui::OverlayFrame)
{
    // TODO: forward the scroll/key events to the window below
    ui->setupUi(this);
}

OverlayFrame::~OverlayFrame()
{
    delete ui;
}

QPushButton* OverlayFrame::button()
{
    return ui->pushButton;
}

QLabel* OverlayFrame::label()
{
    return ui->label;
}
