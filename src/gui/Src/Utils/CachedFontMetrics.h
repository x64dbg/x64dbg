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
        memset(mWidths, 0, sizeof(mWidths));
    }

    int width(const QChar & ch)
    {
        auto unicode = ch.unicode();
        if(unicode >= 0xD800)
        {
            if(unicode >= 0xE000)
                unicode -= 0xE000 - 0xD800;
            else
                // is lonely surrogate
                return mFontMetrics.width(ch);
        }
        if(!mWidths[unicode])
            return mWidths[unicode] = mFontMetrics.width(ch);
        return mWidths[unicode];
    }

    int width(const QString & text)
    {
        int result = 0;
        QChar temp;
        for(const QChar & ch : text)
        {
            if(ch.isLowSurrogate())
                temp = ch;
            else if(ch.isHighSurrogate())
                result += mFontMetrics.width(QString(temp) + ch);
            else
                result += width(ch);
        }
        return result;
    }

private:
    QFontMetrics mFontMetrics;
    uchar mWidths[0x10000 - 0xE000 + 0xD800];
};

#endif // CACHEDFONTMETRICS_H
