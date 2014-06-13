#ifndef APPEARANCEDIALOG_H
#define APPEARANCEDIALOG_H

#include <QDialog>
#include <QColorDialog>
#include <QMessageBox>
#include "Configuration.h"

namespace Ui {
class AppearanceDialog;
}

class AppearanceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AppearanceDialog(QWidget *parent = 0);
    ~AppearanceDialog();

private slots:
    void on_button000000_clicked();
    void on_button000080_clicked();
    void on_button008000_clicked();
    void on_button008080_clicked();
    void on_button800000_clicked();
    void on_button800080_clicked();
    void on_button808000_clicked();
    void on_buttonC0C0C0_clicked();
    void on_button808080_clicked();
    void on_button0000FF_clicked();
    void on_button00FF00_clicked();
    void on_button00FFFF_clicked();
    void on_buttonFF0000_clicked();
    void on_buttonFF00FF_clicked();
    void on_buttonFFFF00_clicked();
    void on_buttonFFFFFF_clicked();
    void on_buttonBackground000000_clicked();
    void on_buttonBackgroundC0C0C0_clicked();
    void on_buttonBackgroundFFFFFF_clicked();
    void on_buttonBackground00FFFF_clicked();
    void on_buttonBackground00FF00_clicked();
    void on_buttonBackgroundFF0000_clicked();
    void on_buttonBackgroundFFFF00_clicked();
    void on_buttonBackgroundNone_clicked();
    void on_editBackgroundColor_textChanged(const QString &arg1);
    void on_editColor_textChanged(const QString &arg1);
    void on_buttonColor_clicked();
    void on_buttonBackgroundColor_clicked();
    void on_buttonSave_clicked();
    void on_listColorNames_itemSelectionChanged();
    void defaultValueSlot();
    void currentSettingSlot();

    void on_buttonCancel_clicked();

private:
    Ui::AppearanceDialog *ui;

    struct ColorInfo
    {
        QString propertyName;
        QString colorName;
        QString backgroundColorName;
    };

    QList<ColorInfo> colorInfoList;
    int colorInfoIndex;
    QMap<QString, QColor>* colorMap;
    QMap<QString, QColor> colorBackupMap;

    QAction* defaultValueAction;
    QAction* currentSettingAction;

    void colorInfoListAppend(QString propertyName, QString colorName, QString backgroundColorName);
    void colorInfoListInit();
};

#endif // APPEARANCEDIALOG_H
