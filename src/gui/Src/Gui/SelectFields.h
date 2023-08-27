#pragma once

#include <QDialog>

class QListWidget;

namespace Ui
{
    class SelectFields;
}

class SelectFields : public QDialog
{
    Q_OBJECT

public:
    explicit SelectFields(QWidget* parent = nullptr);
    QListWidget* GetList();
    ~SelectFields();

private:
    Ui::SelectFields* ui;
};
