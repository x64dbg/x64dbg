#pragma once

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
        mHeight = mFontMetrics.height();
    }

    int width(const QChar & ch)
    {
        auto unicode = ch.unicode();
        if(unicode >= 0xD800)
        {
            if(unicode >= 0xE000)
            {
                unicode -= 0xE000 - 0xD800;
            }
            else
            {
                // is lonely surrogate
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
                return mFontMetrics.horizontalAdvance(ch);
#else
                return mFontMetrics.width(ch);
#endif // QT_VERSION
            }
        }
        if(!mWidths[unicode])
        {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
            return mWidths[unicode] = mFontMetrics.horizontalAdvance(ch);
#else
            return mWidths[unicode] = mFontMetrics.width(ch);
#endif // QT_VERSION
        }
        return mWidths[unicode];
    }

    int width(const QString & text)
    {
        int result = 0;
        QChar temp;
        for(const QChar & ch : text)
        {
            if(ch.isHighSurrogate())
            {
                temp = ch;
            }
            else if(ch.isLowSurrogate())
            {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
                result += mFontMetrics.horizontalAdvance(QString(temp) + ch);
#else
                result += mFontMetrics.width(QString(temp) + ch);
#endif // QT_VERSION
            }
            else
            {
                result += width(ch);
            }
        }
        return result;
    }

    int height()
    {
        return mHeight;
    }

private:
    QFontMetrics mFontMetrics;
    uchar mWidths[0x10000 - 0xE000 + 0xD800];
    int mHeight;
};
