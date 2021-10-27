#pragma once

#include <QPlainTextEdit>
#include "BridgeResult.h"

class NotepadView : public QPlainTextEdit
{
    Q_OBJECT
public:
    NotepadView(QWidget* parent, BridgeResult::Type type);

public slots:
    void updateStyle();
    void setNotes(const QString text);
    void getNotes(void* ptr);

private:
    BridgeResult::Type mType;
};
