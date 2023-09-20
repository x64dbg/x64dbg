#pragma once

#include <QDialog>

namespace Ui
{
    class ComboBoxDialog;
}

class ComboBoxDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ComboBoxDialog(QWidget* parent = nullptr);
    ~ComboBoxDialog();
    bool bChecked;
    QString currentText();
    void setEditable(bool editable);
    void setItems(const QStringList & items);
    // Minimum number of characters that should fit into the combobox.
    // Use for large models, so that the length is not computed from its items.
    void setMinimumContentsLength(int characters);
    void setText(const QString & text);
    void setPlaceholderText(const QString & text);
    void enableCheckBox(bool bEnable);
    void setCheckBox(bool bSet);
    void setCheckBoxText(const QString & text);

private slots:
    void on_checkBox_toggled(bool checked);

private:
    Ui::ComboBoxDialog* ui;
};
