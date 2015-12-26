#ifndef ENTROPYDIALOG_H
#define ENTROPYDIALOG_H

#include <QDialog>

namespace Ui
{
    class EntropyDialog;
}

class EntropyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntropyDialog(QWidget* parent = 0);
    ~EntropyDialog();
    void GraphMemory(const unsigned char* data, int dataSize, QColor color = Qt::darkGreen);
    void GraphFile(const QString & fileName, QColor color = Qt::darkGreen);

private:
    Ui::EntropyDialog* ui;
    int mBlockSize;
    int mPointCount;
    bool mInitialized;

    void initializeGraph();
};

#endif // ENTROPYDIALOG_H
