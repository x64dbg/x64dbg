#pragma once

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "Bridge.h"

struct TypeDescriptor
{
    TYPEDESCRIPTOR type = {};
    QString name;
    QString typeName;
};
Q_DECLARE_METATYPE(TypeDescriptor)

class TypeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit TypeWidget(QWidget* parent = nullptr);
    ~TypeWidget() = default;

    void clearTypes();
    QTreeWidgetItem* typeAddNode(QTreeWidgetItem* parent, const TYPEDESCRIPTOR* type);
    void saveWindowSettings(const QString & settingSection);
    void loadWindowSettings(const QString & settingSection);

    // Column indices
    enum ColumnIndex
    {
        ColField = 0,
        ColOffset = 1,
        ColAddress = 2,
        ColSize = 3,
        ColValue = 4
    };

public slots:
    void colorsUpdatedSlot();
    void fontsUpdatedSlot();
    void updateValuesSlot();

private:
    QString highlightTypeName(QString name) const;

    QTreeWidgetItem* mScrollItem = nullptr;
    int mInsertIndex = -1;
    QColor mTextColor;
};