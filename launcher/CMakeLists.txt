set(SRCS
    AboutDlg.cpp
    AboutDlg.h
    CMakeLists.txt
    DashFactionLauncher.rc
    LauncherApp.cpp
    LauncherApp.h
    LauncherCommandLineInfo.h
    main.cpp
    MainDlg.cpp
    MainDlg.h
    OptionsDlg.cpp
    OptionsDlg.h
    OptionsDisplayDlg.cpp
    OptionsDisplayDlg.h
    OptionsGraphicsDlg.cpp
    OptionsGraphicsDlg.h
    OptionsMiscDlg.cpp
    OptionsMiscDlg.h
    OptionsAudioDlg.cpp
    OptionsAudioDlg.h
    OptionsMultiplayerDlg.cpp
    OptionsMultiplayerDlg.h
    OptionsInputDlg.cpp
    OptionsInputDlg.h
    OptionsInterfaceDlg.cpp
    OptionsInterfaceDlg.h
    res
    resource.h
    stdafx.cpp
    stdafx.h
    targetver.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_executable(DashFactionLauncher WIN32 ${SRCS})

target_compile_features(DashFactionLauncher PUBLIC cxx_std_20)
set_target_properties(DashFactionLauncher PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(DashFactionLauncher)
setup_debug_info(DashFactionLauncher)

target_compile_definitions(DashFactionLauncher PRIVATE XLOG_NO_MACROS)

target_include_directories(DashFactionLauncher SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
    ${CMAKE_SOURCE_DIR}/vendor/win32xx
)

target_link_libraries(DashFactionLauncher
    LauncherCommon
    Xlog
    CrashHandlerStub
    comctl32
)
