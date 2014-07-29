#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFile>
#include <QString>
#include <QColor>
#include <QMap>
#include <QDebug>
#include <QObject>
#include <QKeySequence>
#include "Bridge.h"

#define Config() (Configuration::instance())
#define ConfigColor(x) (Config()->getColor(x))
#define ConfigBool(x,y) (Config()->getBool(x,y))
#define ConfigUint(x,y) (Config()->getUint(x,y))
#define ConfigFont(x) (Config()->getFont(x))
#define ConfigShortcut(x) (Config()->getShortcut(x).Hotkey)




// X64RUNHotkeys
// ^     ^
namespace XH
{
enum ShortcutId
{
    FILE_OPEN=0,
    APP_EXIT,
    VIEW_CPU,
    VIEW_MEMORY,
    VIEW_LOG,
    VIEW_BREAKPOINTS,
    VIEW_SCRIPT,
    VIEW_SYMINFO,
    VIEW_REFERENCES,
    VIEW_THREADS,
    VIEW_PATCHES,
    VIEW_COMMENTS,
    VIEW_LABELS,
    VIEW_BOOKMARKS,
    VIEW_FUNCTIONS,
    DEBUG_RUN, DEBUG_SKIPEXC,DEBUG_RUNUNTIL,DEBUG_PAUSE,DEBUG_RESTART,DEBUG_CLOSE,DEBUG_STEPIN,DEBUG_STEPINSKIP,DEBUG_STEPOVER,DEBUG_STEPOVERSKIP,DEBUG_EXECTILL,DEBUG_EXECTILLSKIP,DEBUG_COMMAND
};

struct Shortcut
{
    ShortcutId Id;
    QString Name;
    QKeySequence Hotkey;

    Shortcut(int i,QString n,int h) : Id(static_cast<XH::ShortcutId>(i)),Name(n),Hotkey(QKeySequence(h)) {}
    Shortcut() : Id(),Name(""),Hotkey(NULL) {}

};
};

class Configuration : public QObject
{
    Q_OBJECT
public:
    static Configuration* mPtr;
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

    const QColor getColor(const QString id) const;
    const bool getBool(const QString category, const QString id) const;
    void setBool(const QString category, const QString id, const bool b);
    const uint_t getUint(const QString category, const QString id) const;
    void setUint(const QString category, const QString id, const uint_t i);
    const QFont getFont(const QString id) const;
    const XH::Shortcut getShortcut(const XH::ShortcutId key_id) const;
    void setShortcut(const XH::ShortcutId key_id, const int key_sequence);

    //default setting maps
    QMap<QString, QColor> defaultColors;
    QMap<QString, QMap<QString, bool>> defaultBools;
    QMap<QString, QMap<QString, uint_t>> defaultUints;
    QMap<QString, QFont> defaultFonts;
    QMap<XH::ShortcutId,XH::Shortcut> defaultShortcuts;

    //public variables
    QMap<QString, QColor> Colors;
    QMap<QString, QMap<QString, bool>> Bools;
    QMap<QString, QMap<QString, uint_t>> Uints;
    QMap<QString, QFont> Fonts;
    QMap<XH::ShortcutId,XH::Shortcut> Shortcuts;


signals:
    void colorsUpdated();
    void fontsUpdated();
    void shortcutsUpdated();

private:
    QColor colorFromConfig(const QString id);
    bool colorToConfig(const QString id, const QColor color);
    bool boolFromConfig(const QString category, const QString id);
    bool boolToConfig(const QString category, const QString id, bool bBool);
    uint_t uintFromConfig(const QString category, const QString id);
    bool uintToConfig(const QString category, const QString id, uint_t i);
    QFont fontFromConfig(const QString id);
    bool fontToConfig(const QString id, const QFont font);
    QString shortcutFromConfig(const int id);
    bool shortcutToConfig(const int id, const QKeySequence shortcut);
};

#endif // CONFIGURATION_H
