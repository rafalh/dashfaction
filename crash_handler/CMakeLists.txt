file(GLOB SRCS *.cpp *.h)
add_executable(CrashHandler WIN32 ${SRCS})
target_link_libraries(CrashHandler psapi zip zlib wininet)
include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
	${CMAKE_SOURCE_DIR}/vendor/zlib
	${CMAKE_SOURCE_DIR}/vendor/zlib/contrib/minizip
)
