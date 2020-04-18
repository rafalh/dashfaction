if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

add_library(CrashHandlerStub STATIC
    crash_handler_stub.cpp
    WatchDogTimer.cpp
    include/crash_handler_stub.h
    include/crash_handler_stub/custom_exceptions.h
    include/crash_handler_stub/WatchDogTimer.h
)

target_compile_features(CrashHandlerStub PUBLIC cxx_std_17)
set_target_properties(CrashHandlerStub PROPERTIES CXX_EXTENSIONS NO)

target_include_directories(CrashHandlerStub PUBLIC include)

target_link_libraries(CrashHandlerStub
    Logger
)
