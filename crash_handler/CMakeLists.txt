include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
	${CMAKE_SOURCE_DIR}/vendor/zlib
	${CMAKE_SOURCE_DIR}/vendor/zlib/contrib/minizip
)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS *.cpp *.h)
add_executable(CrashHandler WIN32 ${SRCS})
set_property(TARGET CrashHandler PROPERTY CXX_STANDARD 17)

target_link_libraries(CrashHandler
    Common
    psapi
    zip
    zlib
    wininet
)
