#pragma once

#include <QDialog>
#include <sys/types.h>

class StdSearchListView;
class QAction;

class AttachDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachDialog(QWidget* parent = nullptr);
    ~AttachDialog() override;

    [[nodiscard]] pid_t selectedPid() const { return mSelectedPid; }

private slots:
    void refresh();
    void on_btnAttach_clicked();

private:
    enum
    {
        ColPid = 0,
        ColName,
        ColPath,
        ColCommandLine,
    };

    // Kept in the Name column userdata so a click can say why a row is refused.
    enum Attachable
    {
        AttachOk = 0,
        AttachWrongArch,
        AttachUnknownArch,
        AttachTraced,
    };

    StdSearchListView* mSearchListView = nullptr;
    QAction* mRefreshAction = nullptr;
    pid_t mSelectedPid = 0;
};
