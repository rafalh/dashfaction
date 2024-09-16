add_library(Xlog STATIC
    include/xlog/Appender.h
    include/xlog/ConsoleAppender.h
    include/xlog/FileAppender.h
    include/xlog/Formatter.h
    include/xlog/Level.h
    include/xlog/Logger.h
    include/xlog/LoggerConfig.h
    include/xlog/LogStream.h
    include/xlog/NullStream.h
    include/xlog/SimpleFormatter.h
    include/xlog/Win32Appender.h
    include/xlog/xlog.h
    src/LoggerConfig.cpp
    src/FileAppender.cpp
    src/SimpleFormatter.cpp
    src/Win32Appender.cpp
)
target_compile_features(Xlog PUBLIC cxx_std_20)
set_target_properties(Xlog PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(Xlog)

target_include_directories(Xlog PUBLIC include)
