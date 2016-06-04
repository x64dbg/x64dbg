#ifndef CACHEDFONTMETRICS_H
#define CACHEDFONTMETRICS_H

#include <QObject>
#include <QFont>
#include <QFontMetrics>

class CachedFontMetrics : public QObject
{
    Q_OBJECT
public:
    explicit CachedFontMetrics(QObject* parent, const QFont & font)
        : QObject(parent),
          mFontMetrics(font)
    {
    }

    int width(const QChar & ch)
    {
        auto unicode = ch.unicode();
        if(!mWidths[unicode])
            return mWidths[unicode] = mFontMetrics.width(ch);
        return mWidths[unicode];
    }

    int width(const QString & text)
    {
        int result = 0;
        for(const QChar & ch : text)
            result += width(ch);
        return result;
    }

private:
    QFontMetrics mFontMetrics;
    ushort mWidths[65536];
};

#endif // CACHEDFONTMETRICS_H
