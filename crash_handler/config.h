// Config
// Information Level: 0 smallest - 2 - biggest
#ifdef NDEBUG
#define CRASHHANDLER_DMP_LEVEL 0
#else
#define CRASHHANDLER_DMP_LEVEL 0
#endif

#define CRASHHANDLER_WEBSVC_ENABLED 1
#define CRASHHANDLER_WEBSVC_URL "https://dashfactionapi.rafalh.dev/crash-report"
#define CRASHHANDLER_WEBSVC_AGENT "DashFaction"
