#ifndef CLOGENTRY_H
#define CLOGENTRY_H

#include <sstream>
#include "ELogLevel.h"

class CLogger;

class CLogEntry
{
public:
    CLogEntry(ELogLevel lvl, CLogger &logger):
        level(lvl), logger(logger) {}

    CLogEntry(const CLogEntry &e):
        level(e.level), logger(e.logger) {}

    template<typename T> CLogEntry &operator<<(const T &v)
    {
        stream << v;
        return *this;
    }

    ~CLogEntry();

private:
    ELogLevel level;
    CLogger &logger;
    std::ostringstream stream;
};

#endif // CLOGENTRY_H
