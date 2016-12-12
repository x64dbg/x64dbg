#include "LogStatusLabel.h"
#include <QTextDocument>

LogStatusLabel::LogStatusLabel(QStatusBar* parent) : QLabel(parent)
{
    this->setFrameStyle(QFrame::Sunken | QFrame::Panel); //sunken style
    this->setStyleSheet("QLabel { background-color : #C0C0C0; }");

    this->setTextFormat(Qt::PlainText);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    connect(Bridge::getBridge(), SIGNAL(addMsgToLog(QByteArray)), this, SLOT(logUpdateUtf8(QByteArray)));
    connect(Bridge::getBridge(), SIGNAL(addMsgToStatusBar(QString)), this, SLOT(logUpdate(QString)));
    connect(Bridge::getBridge(), SIGNAL(getActiveView(ACTIVEVIEW*)), this, SLOT(getActiveView(ACTIVEVIEW*)));
    connect(QApplication::instance(), SIGNAL(focusChanged(QWidget*, QWidget*)), this, SLOT(focusChanged(QWidget*, QWidget*)));
}

void LogStatusLabel::logUpdate(QString message)
{
    //TODO: This subroutine can be optimized
    if(!message.length())
        return;
    labelText += message.replace("\r\n", "\n");
    QStringList lineList = labelText.split('\n');
    labelText = lineList.last(); //if the last character is a newline this will be an empty string
    QString finalLabel;
    for(int i = 0; i < lineList.length(); i++)
    {
        const QString & line = lineList[lineList.size() - i - 1];
        if(line.length()) //set the last non-empty string from the split
        {
            finalLabel = line;
            break;
        }
    }
    setText(finalLabel);
}

void LogStatusLabel::logUpdateUtf8(QByteArray message)
{
    logUpdate(QString::fromUtf8(message));
}

void LogStatusLabel::focusChanged(QWidget* old, QWidget* now)
{
    if(old && now && QString(now->metaObject()->className()) == QString("CPUWidget"))
    {
        old->setFocus();
        return;
    }
}

void LogStatusLabel::getActiveView(ACTIVEVIEW* active)
{
    auto findTitle = [](QWidget * w, void* & hwnd) -> QString
    {
        if(!w)
            return "(null)";
        if(!w->windowTitle().length())
        {
            auto p = w->parentWidget();
            if(p && p->windowTitle().length())
            {
                hwnd = (void*)p->winId();
                return p->windowTitle();
            }
        }
        hwnd = (void*)w->winId();
        return w->windowTitle();
    };
    auto className = [](QWidget * w, void* & hwnd) -> QString
    {
        if(!w)
            return "(null)";
        hwnd = (void*)w->winId();
        return w->metaObject()->className();
    };

    memset(active, 0, sizeof(ACTIVEVIEW));
    QWidget* now = QApplication::focusWidget();
    strncpy_s(active->title, findTitle(now, active->titleHwnd).toUtf8().constData(), _TRUNCATE);
    strncpy_s(active->className, className(now, active->classHwnd).toUtf8().constData(), _TRUNCATE);
    Bridge::getBridge()->setResult();
}
