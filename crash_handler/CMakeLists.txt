set(SRCS
    main.cpp
    CrashReportApp.cpp
    CrashReportApp.h
    MainDlg.cpp
    MainDlg.h
    MiniDumpHelper.cpp
    MiniDumpHelper.h
    TextDumpHelper.h
    ZipHelper.h
    res/resources.rc
    res/resources2.rc
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_executable(CrashHandler WIN32 ${SRCS})

target_compile_features(CrashHandler PRIVATE cxx_std_17)
set_target_properties(CrashHandler PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(CrashHandler)
setup_debug_info(CrashHandler)

target_include_directories(CrashHandler SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/zlib
    ${CMAKE_SOURCE_DIR}/vendor/zlib/contrib/minizip
    ${CMAKE_SOURCE_DIR}/vendor/win32xx
)

target_link_libraries(CrashHandler
    Common
    psapi
    comctl32
    zip
    zlib
)

