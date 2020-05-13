#include <common/version.h>

// Config
#define CRASHHANDLER_LOG_PATH "logs/DashFaction.log"
#define CRASHHANDLER_DMP_FILENAME "logs/DashFaction.dmp"
#define CRASHHANDLER_TARGET_DIR "logs"
#define CRASHHANDLER_TARGET_NAME "DashFaction-crash.zip"
//#define CRASHHANDLER_MSG "Game has crashed!\nTo help resolve the problem please send " CRASHHANDLER_TARGET_NAME " file
//from logs subdirectory in RedFaction directory to " PRODUCT_NAME " author."
#define CRASHHANDLER_MSG                                                                                     \
    "Game has crashed!\nReport has been generated in " CRASHHANDLER_TARGET_DIR "\\" CRASHHANDLER_TARGET_NAME \
    ".\nDo you want to send it to " PRODUCT_NAME " author to help resolve the problem?"
// Information Level: 0 smallest - 2 - biggest
#ifdef NDEBUG
#define CRASHHANDLER_DMP_LEVEL 0
#else
#define CRASHHANDLER_DMP_LEVEL 0
#endif

#define CRASHHANDLER_WEBSVC_ENABLED 1
#define CRASHHANDLER_WEBSVC_URL "https://ravin.tk/api/rf/dashfaction/crashreport.php"
#define CRASHHANDLER_WEBSVC_AGENT "DashFaction"
