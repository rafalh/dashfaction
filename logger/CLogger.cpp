#include "CLogger.h"
#include "CLoggerConfig.h"

CLogger::CLogger(const std::string &name):
    name(name), conf(CLoggerConfig::root()) {}

CLogger::CLogger(const std::string &name, CLoggerConfig &conf):
    name(name), conf(conf) {}
