#include "LabeledSplitterDetachedWindow.h"


LabeledSplitterDetachedWindow::LabeledSplitterDetachedWindow(QWidget* parent, LabeledSplitter* splitterwidget)
    : QMainWindow(parent),
      index(0)
{
    m_SplitterWidget = splitterwidget;
}

LabeledSplitterDetachedWindow::~LabeledSplitterDetachedWindow()
{
}

void LabeledSplitterDetachedWindow::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event);

    emit OnClose(this);
}
