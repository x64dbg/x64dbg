#include "NotepadView.h"
#include "Configuration.h"
#include "Bridge.h"
#include <QMessageBox>

NotepadView::NotepadView(QWidget* parent, BridgeResult::Type type) : QPlainTextEdit(parent), mType(type)
{
    updateStyle();
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateStyle()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateStyle()));
}

void NotepadView::updateStyle()
{
    setFont(ConfigFont("Log"));
    setStyleSheet(QString("QPlainTextEdit { color: %1; background-color: %2 }").arg(ConfigColor("AbstractTableViewTextColor").name(), ConfigColor("AbstractTableViewBackgroundColor").name()));
}

void NotepadView::setNotes(const QString text)
{
    setPlainText(text);
}

void NotepadView::getNotes(void* ptr)
{
    QByteArray text = toPlainText().replace('\n', "\r\n").toUtf8();
    char* result = 0;
    if(text.length())
    {
        result = (char*)BridgeAlloc(text.length() + 1);
        strcpy_s(result, text.length() + 1, text.constData());
    }
    *(char**)ptr = result;
    Bridge::getBridge()->setResult(mType);
}
