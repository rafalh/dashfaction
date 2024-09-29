set(SRCS
    crash_handler_stub.cpp
    WatchDogTimer.cpp
    include/crash_handler_stub.h
    include/crash_handler_stub/custom_exceptions.h
    include/crash_handler_stub/WatchDogTimer.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(CrashHandlerStub STATIC ${SRCS})

target_compile_features(CrashHandlerStub PUBLIC cxx_std_20)
set_target_properties(CrashHandlerStub PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(CrashHandlerStub)

target_include_directories(CrashHandlerStub PUBLIC include)

target_link_libraries(CrashHandlerStub
    Xlog
)
