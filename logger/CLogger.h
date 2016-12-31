#ifndef CLOGGER_H
#define CLOGGER_H

#include <string>
#include "CLogEntry.h"
#include "CLoggerConfig.h"
#include "CNullStream.h"

class CLogger
{
public:
    CLogger(const std::string &name = "");
    CLogger(const std::string &name, CLoggerConfig &conf);

    static CLogger &root()
    {
        static CLogger rootLogger;
        return rootLogger;
    }

    void output(ELogLevel lvl, const std::string &str)
    {
        conf.output(lvl, name, str);
    }

    CLogEntry get(ELogLevel lvl)
    {
        return CLogEntry(lvl, *this);
    }

    CLogEntry err()
    {
        return get(LOG_LVL_ERROR);
    }

    CLogEntry warn()
    {
        return get(LOG_LVL_WARNING);
    }
    
    CLogEntry info()
    {
        return get(LOG_LVL_INFO);
    }

#ifndef NDEBUG
    CLogEntry trace()
    {
        return get(LOG_LVL_TRACE);
    }
#else
    CNullStream trace()
    {
        return CNullStream();
    }
#endif

private:
    std::string name;
    CLoggerConfig &conf;
};

#endif // CLOGGER_H
