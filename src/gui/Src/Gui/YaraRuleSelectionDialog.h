#ifndef YARARULESELECTIONDIALOG_H
#define YARARULESELECTIONDIALOG_H

#include <QDialog>

namespace Ui
{
    class YaraRuleSelectionDialog;
}

class YaraRuleSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit YaraRuleSelectionDialog(QWidget* parent = 0, const QString & title = "Yara");
    ~YaraRuleSelectionDialog();
    QString getSelectedFile();

private slots:
    void on_buttonDirectory_clicked();
    void on_buttonFile_clicked();
    void on_buttonSelect_clicked();

private:
    Ui::YaraRuleSelectionDialog* ui;
    QList<QPair<QString, QString>> ruleFiles;
    QString rulesDirectory;
    QString selectedFile;

    void enumRulesDirectory();
};

#endif // YARARULESELECTIONDIALOG_H
