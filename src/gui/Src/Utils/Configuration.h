#pragma once

#include <QObject>
#include <QKeySequence>
#include <QMap>
#include <QPointer>
#include <QColor>
#include <QFont>
#include "Imports.h"
#include "MenuBuilder.h"

enum ValueStyleType
{
    ValueStyleDefault = 0,
    ValueStyleC = 1,
    ValueStyleMASM = 2
};

// TODO: declare AppearanceDialog and SettingsDialog entries here, so that you only have to do it in once place
#define Config() (Configuration::instance())
#define ConfigColor(x) (Config()->getColor(x))
#define ConfigBool(x,y) (Config()->getBool(x,y))
#define ConfigUint(x,y) (Config()->getUint(x,y))
#define ConfigFont(x) (Config()->getFont(x))
#define ConfigShortcut(x) (Config()->getShortcut(x).Hotkey)
#define ConfigHScrollBarStyle() "QScrollBar:horizontal{border:1px solid grey;background:#f1f1f1;height:10px}QScrollBar::handle:horizontal{background:#aaaaaa;min-width:20px;margin:1px}QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;height:0}"
#define ConfigVScrollBarStyle() "QScrollBar:vertical{border:1px solid grey;background:#f1f1f1;width:10px}QScrollBar::handle:vertical{background:#aaaaaa;min-height:20px;margin:1px}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{width:0;height:0}"

class QAction;
class QWheelEvent;

class Configuration : public QObject
{
    Q_OBJECT
public:
    //Structures
    struct Shortcut
    {
        QString Name;
        QKeySequence Hotkey;
        bool GlobalShortcut;

        Shortcut(QString name = QString(), QString hotkey = QString(), bool global = false)
            : Name(name), Hotkey(hotkey, QKeySequence::PortableText), GlobalShortcut(global) { }

        Shortcut(std::initializer_list<QString> names, QString hotkey = QString(), bool global = false)
            : Shortcut(QStringList(names).join(" -> "), hotkey, global) { }
    };

    //Functions
    Configuration();
    static Configuration* instance();
    void load();
    void save();
    void readColors();
    void writeColors();
    void readBools();
    void writeBools();
    void readUints();
    void writeUints();
    void readFonts();
    void writeFonts();
    void readShortcuts();
    void writeShortcuts();
    bool registerMenuBuilder(MenuBuilder* menu, size_t count);
    bool registerMainMenuStringList(QList<QAction*>* menu);
    void unregisterMenuBuilder(MenuBuilder* meun);

    const QColor getColor(const QString & id) const;
    const bool getBool(const QString & category, const QString & id) const;
    void setBool(const QString & category, const QString & id, const bool b);
    const duint getUint(const QString & category, const QString & id) const;
    void setUint(const QString & category, const QString & id, const duint i);
    const QFont getFont(const QString & id) const;
    const Shortcut getShortcut(const QString & key_id) const;
    void setShortcut(const QString & key_id, const QKeySequence key_sequence);
    void setPluginShortcut(const QString & key_id, QString description, QString defaultShortcut, bool global);
    void loadWindowGeometry(QWidget* window);
    void saveWindowGeometry(QWidget* window);

    void zoomFont(const QString & fontName, QWheelEvent* event);

    //default setting maps
    QMap<QString, QColor> defaultColors;
    QMap<QString, QMap<QString, bool>> defaultBools;
    QMap<QString, QMap<QString, duint>> defaultUints;
    QMap<QString, QFont> defaultFonts;
    QMap<QString, Shortcut> defaultShortcuts;

    //public variables
    QMap<QString, QColor> Colors;
    QMap<QString, QMap<QString, bool>> Bools;
    QMap<QString, QMap<QString, duint>> Uints;
    QMap<QString, QFont> Fonts;
    QMap<QString, Shortcut> Shortcuts;

    //custom menu maps
    struct MenuMap
    {
        QList<QAction*>* mainMenuList;
        QPointer<MenuBuilder> builder;
        int type; // 0: builder; 1: mainMenuList
        size_t count;

        MenuMap() { }
        MenuMap(QList<QAction*>* mainMenuList, size_t count)
            : mainMenuList(mainMenuList), type(1), count(count) { }
        MenuMap(MenuBuilder* builder, size_t count)
            : builder(builder), type(0), count(count) { }
    };

    QList<MenuMap> NamedMenuBuilders;

    static Configuration* mPtr;

signals:
    void colorsUpdated();
    void fontsUpdated();
    void guiOptionsUpdated();
    void shortcutsUpdated();
    void tokenizerConfigUpdated();
    void disableAutoCompleteUpdated();

private:
    QColor colorFromConfig(const QString & id);
    bool colorToConfig(const QString & id, const QColor color);
    bool boolFromConfig(const QString & category, const QString & id);
    bool boolToConfig(const QString & category, const QString & id, bool bBool);
    duint uintFromConfig(const QString & category, const QString & id);
    bool uintToConfig(const QString & category, const QString & id, duint i);
    QFont fontFromConfig(const QString & id);
    bool fontToConfig(const QString & id, const QFont font);
    QString shortcutFromConfig(const QString & id);
    bool shortcutToConfig(const QString & id, const QKeySequence shortcut);

    mutable bool noMoreMsgbox;
};