#pragma once

#include <QAction>
#include <QDialog>
#include <QMap>
#include <QColorDialog>
#include <QLineEdit>

namespace Ui
{
    class AppearanceDialog;
}

class QTreeWidgetItem;

class AppearanceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AppearanceDialog(QWidget* parent = 0);
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
    void on_editBackgroundColor_textChanged(const QString & arg1);
    void on_editColor_textChanged(const QString & arg1);
    void on_buttonColor_clicked();
    void on_buttonBackgroundColor_clicked();
    void on_buttonSave_clicked();
    void on_listColorNames_itemSelectionChanged();
    void defaultValueSlot();
    void currentSettingSlot();
    void on_fontAbstractTables_currentFontChanged(const QFont & f);
    void on_fontAbstractTablesStyle_currentIndexChanged(int index);
    void on_fontAbstractTablesSize_currentIndexChanged(const QString & arg1);
    void on_fontDisassembly_currentFontChanged(const QFont & f);
    void on_fontDisassemblyStyle_currentIndexChanged(int index);
    void on_fontDisassemblySize_currentIndexChanged(const QString & arg1);
    void on_fontHexDump_currentFontChanged(const QFont & f);
    void on_fontHexDumpStyle_currentIndexChanged(int index);
    void on_fontHexDumpSize_currentIndexChanged(const QString & arg1);
    void on_fontStack_currentFontChanged(const QFont & f);
    void on_fontStackStyle_currentIndexChanged(int index);
    void on_fontStackSize_currentIndexChanged(const QString & arg1);
    void on_fontRegisters_currentFontChanged(const QFont & f);
    void on_fontRegistersStyle_currentIndexChanged(int index);
    void on_fontRegistersSize_currentIndexChanged(const QString & arg1);
    void on_fontHexEdit_currentFontChanged(const QFont & f);
    void on_fontHexEditStyle_currentIndexChanged(int index);
    void on_fontHexEditSize_currentIndexChanged(const QString & arg1);
    void on_fontLog_currentFontChanged(const QFont & f);
    void on_fontLogStyle_currentIndexChanged(int index);
    void on_fontLogSize_currentIndexChanged(const QString & arg1);
    void on_buttonApplicationFont_clicked();
    void on_buttonFontDefaults_clicked();
    void rejectedSlot();
    void colorSelectionChangedSlot(QColor color);

private:
    Ui::AppearanceDialog* ui;
    QLineEdit* colorLineEdit = nullptr;

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
    QMap<QString, QFont>* fontMap;
    QMap<QString, QFont> fontBackupMap;

    QAction* defaultValueAction;
    QAction* currentSettingAction;
    QTreeWidgetItem* currentCategory;

    bool isInit;

    void colorInfoListCategory(QString categoryName);
    void colorInfoListAppend(QString propertyName, QString colorName, QString backgroundColorName);
    void colorInfoListInit();
    void fontInit();

    void selectColor(QLineEdit* lineEdit, QColorDialog::ColorDialogOptions options = QColorDialog::ColorDialogOptions());
    static QString colorToString(const QColor & color);
};
