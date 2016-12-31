#include "CLogEntry.h"
#include "CLogger.h"

CLogEntry::~CLogEntry()
{
    logger.output(level, stream.str());
}
