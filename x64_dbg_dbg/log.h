#ifndef _LOG_H
#define _LOG_H

#include <sstream>

// a Qt's QDebug like message logging
// usage: "log() << "hi" << "there";
class log
{
public:
    log();
    ~log();

public:
    template<class T>
    inline log & operator<<(const T & x)
    {
        // accumulate messages
        message << x;
        return *this;
    }

private:
    std::ostringstream message;
};

#endif _LOG_H