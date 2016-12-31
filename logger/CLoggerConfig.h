#ifndef CLOGGERCONFIG_H
#define CLOGGERCONFIG_H

#include <fstream>
#include <mutex>
#include "ELogLevel.h"

class CLoggerConfig
{
public:
    CLoggerConfig();

    static CLoggerConfig &root()
    {
        static CLoggerConfig conf;
        return conf;
    }

    void setOutputFile(const std::string &filename, bool append = false);
    void setFlushEnabled(bool enabled);
    void output(ELogLevel lvl, const std::string &loggerName, const std::string &str);

    void setComponent(const std::string &name)
    {
        component = name;
    }

private:
    std::ofstream outputFile;
    std::mutex mutex;
    std::string component;
    bool flush;
};

#endif // CLOGGERCONFIG_H
