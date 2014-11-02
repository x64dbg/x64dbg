#ifndef SELECTFIELDS_H
#define SELECTFIELDS_H

#include <QDialog>
#include <QListWidget>

namespace Ui
{
class SelectFields;
}

class SelectFields : public QDialog
{
    Q_OBJECT

public:
    explicit SelectFields(QWidget* parent = 0);
    QListWidget* GetList(void);
    ~SelectFields();

private:
    Ui::SelectFields* ui;
};

#endif // SELECTFIELDS_H
