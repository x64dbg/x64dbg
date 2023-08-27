#pragma once

#include <QDialog>

class QTableWidget;

namespace Ui
{
    class FavouriteTools;
}

class FavouriteTools : public QDialog
{
    Q_OBJECT

public:
    explicit FavouriteTools(QWidget* parent = nullptr);
    ~FavouriteTools();
public slots:
    void on_btnAddFavouriteTool_clicked();
    void on_btnEditFavouriteTool_clicked();
    void on_btnRemoveFavouriteTool_clicked();
    void on_btnDescriptionFavouriteTool_clicked();
    void on_btnUpFavouriteTool_clicked();
    void on_btnDownFavouriteTool_clicked();
    void on_btnAddFavouriteScript_clicked();
    void on_btnEditFavouriteScript_clicked();
    void on_btnRemoveFavouriteScript_clicked();
    void on_btnDescriptionFavouriteScript_clicked();
    void on_btnUpFavouriteScript_clicked();
    void on_btnDownFavouriteScript_clicked();
    void on_btnAddFavouriteCommand_clicked();
    void on_btnEditFavouriteCommand_clicked();
    void on_btnRemoveFavouriteCommand_clicked();
    void on_btnUpFavouriteCommand_clicked();
    void on_btnDownFavouriteCommand_clicked();
    void on_btnClearShortcut_clicked();
    void onListSelectionChanged();
    void tabChanged(int i);
    void on_shortcutEdit_askForSave();
    void on_btnOK_clicked();

private:
    Ui::FavouriteTools* ui;
    QKeySequence currentShortcut;
    int originalToolsCount;
    int originalScriptCount;
    int originalCommandCount;

    void upbutton(QTableWidget* table);
    void downbutton(QTableWidget* table);

    void setupTools(QString name, QTableWidget* list);
    void updateToolsBtnEnabled();
    void updateScriptsBtnEnabled();
    void updateCommandsBtnEnabled();
};
