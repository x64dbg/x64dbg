#ifndef SIMPLETRACEDIALOG_H
#define SIMPLETRACEDIALOG_H

#include <QDialog>

namespace Ui
{
    class SimpleTraceDialog;
}

class SimpleTraceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SimpleTraceDialog(QWidget* parent = 0);
    ~SimpleTraceDialog();
    void setTraceCommand(const QString & command);

private slots:
    void on_btnOk_clicked();
    void on_btnLogFile_clicked();

    void on_checkBox_clicked();

private:
    Ui::SimpleTraceDialog* ui;
    QString mTraceCommand;
    QString mLogFile;
};

#endif // SIMPLETRACEDIALOG_H
