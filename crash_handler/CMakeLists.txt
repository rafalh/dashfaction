include_directories(
	${CMAKE_SOURCE_DIR}/vendor/zlib
	${CMAKE_SOURCE_DIR}/vendor/zlib/contrib/minizip
)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

set(SRCS
    main.cpp
    MiniDumpHelper.cpp
    MiniDumpHelper.h
    TextDumpHelper.h
    ZipHelper.h
    resources.rc
)
add_executable(CrashHandler WIN32 ${SRCS})

target_compile_features(CrashHandler PRIVATE cxx_std_17)
set_target_properties(CrashHandler PROPERTIES CXX_EXTENSIONS NO)

target_link_libraries(CrashHandler
    Common
    psapi
    zip
    zlib
)
