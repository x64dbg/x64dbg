#pragma once

#include <QDialog>
#include <QKeyEvent>

namespace Ui
{
    class PatchDialogGroupSelector;
}

class PatchDialogGroupSelector : public QDialog
{
    Q_OBJECT

public:
    explicit PatchDialogGroupSelector(QWidget* parent = 0);
    ~PatchDialogGroupSelector();
    void setGroupTitle(const QString & title);
    void setPreviousEnabled(bool enable);
    void setNextEnabled(bool enable);
    void setGroup(int group);
    int group();

signals:
    void groupToggle();
    void groupPrevious();
    void groupNext();

private slots:
    void on_btnToggle_clicked();
    void on_btnPrevious_clicked();
    void on_btnNext_clicked();

protected:
    void keyPressEvent(QKeyEvent* event);

private:
    Ui::PatchDialogGroupSelector* ui;
    int mGroup;
};
