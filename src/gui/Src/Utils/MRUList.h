#pragma once

#include <QObject>
#include <Qlist>

class QMenu;

class MRUList : public QObject
{
    Q_OBJECT
public:
    explicit MRUList(QObject* parent, const char* section, int maxItems = 16);

    void load();
    void save();
    void appendMenu(QMenu* menu);

    void addEntry(QString entry);
    void removeEntry(QString entry);
    QString getEntry(int index);

signals:
    void openFile(QString filename);

private slots:
    void openFileSlot();

private:
    const char* mSection;
    QList<QString> mMRUList;
    int mMaxMRU;
};
