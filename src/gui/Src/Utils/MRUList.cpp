#include "MRUList.h"
#include "Bridge.h"
#include <QMenu>
#include <QFile>

MRUList::MRUList(QObject* parent, const char* section, int maxItems)
    : QObject(parent),
      mSection(section),
      mMaxMRU(maxItems)
{
}

void MRUList::load()
{
    for(int i = 0; i < mMaxMRU; i++)
    {
        char currentFile[MAX_SETTING_SIZE] = "";
        if(!BridgeSettingGet(mSection, QString().sprintf("%.2d", i + 1).toUtf8().constData(), currentFile))
            break;
        if(QString(currentFile).size() && QFile(currentFile).exists())
            mMRUList.push_back(currentFile);
    }
    mMRUList.removeDuplicates();
}

void MRUList::save()
{
    BridgeSettingSet(mSection, 0, 0); //clear
    mMRUList.removeDuplicates();
    int mruSize = mMRUList.size();
    for(int i = 0; i < mruSize; i++)
    {
        if(QFile(mMRUList.at(i)).exists())
            BridgeSettingSet(mSection, QString().sprintf("%.2d", i + 1).toUtf8().constData(), mMRUList.at(i).toUtf8().constData());
    }
}

void MRUList::appendMenu(QMenu* menu)
{
    if(mMaxMRU < 1)
        return;

    /*QList<QAction*> list = menu->actions();
    for(int i = 1; i < list.length(); ++i)
        menu->removeAction(list.at(i));*/

    //add items to list
    if(mMRUList.size() > 0)
    {
        for(int index = 0; index < mMRUList.size(); ++index)
        {
            menu->addAction(new QAction(mMRUList.at(index), this));
            menu->actions().last()->setObjectName(QString("MRU").append(QString::number(index)));
            connect(menu->actions().last(), SIGNAL(triggered()), this, SLOT(openFileSlot()));
        }
    }
}

void MRUList::addEntry(QString entry)
{
    if(!entry.size())
        return;

    //remove duplicate entry if it exists
    removeEntry(entry);
    mMRUList.insert(mMRUList.begin(), entry);
    if(mMRUList.size() > mMaxMRU)
        mMRUList.erase(mMRUList.begin() + mMaxMRU, mMRUList.end());
}

void MRUList::removeEntry(QString entry)
{
    if(!entry.size())
        return;

    for(auto it = mMRUList.begin(); it != mMRUList.end(); ++it)
    {
        if(*it == entry)
        {
            mMRUList.erase(it);
            break;
        }
    }
}

QString MRUList::getEntry(int index)
{
    if(index < mMRUList.size())
        return mMRUList.at(index);

    return QString();
}

void MRUList::openFileSlot()
{
    QAction* fileToOpen = qobject_cast<QAction*>(sender());

    //if sender is from recent list directly open file
    if(fileToOpen && fileToOpen->objectName().startsWith("MRU") && fileToOpen->text().length())
        emit openFile(fileToOpen->text());
}
