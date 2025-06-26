#pragma once

#include <QDialog>
#include "Bridge.h"

class TypeManager;
class QLineEdit;
class QLabel;

namespace Ui
{
    class DisplayTypeDialog;
}

class DisplayTypeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DisplayTypeDialog(QWidget* parent = nullptr);
    ~DisplayTypeDialog();

    static void pickType(QWidget* parent = nullptr, duint defaultAddr = 0);

private slots:
    void on_addressEdit_textChanged(const QString & addressText);

    void updateTypeListSlot();
    void updateTypeWidgetSlot();

private:
    enum
    {
        ColType,
    };

    Ui::DisplayTypeDialog* ui;
    duint mCurrentAddress = 0;

    QString getSelectedType() const;
    void validateAddress();
    void saveWindowSettings();
    void loadWindowSettings();
};