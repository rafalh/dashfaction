#pragma once

#ifndef TOSTRING
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

// clang-format off

#define VERSION_TYPE_DEV      0
#define VERSION_TYPE_ALPHA    1
#define VERSION_TYPE_BETA     2
#define VERSION_TYPE_RELEASE  3

// Variables to be modified during the release process
#define PRODUCT_NAME   "Dash Faction"
#define VERSION_MAJOR  1
#define VERSION_MINOR  6
#define VERSION_PATCH  1
#define VERSION_TYPE   VERSION_TYPE_DEV

// clang-format on

#if VERSION_TYPE == VERSION_TYPE_DEV
#define VERSION_SUFFIX        "-dev"
#elif VERSION_TYPE == VERSION_TYPE_ALPHA
#define VERSION_SUFFIX        "-alpha"
#elif VERSION_TYPE == VERSION_TYPE_BETA
#define VERSION_SUFFIX        "-beta"
#elif VERSION_TYPE == VERSION_TYPE_RELEASE
#define VERSION_SUFFIX        ""
#else
#error Unknown version type
#endif

#define VERSION_STR TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH) VERSION_SUFFIX
#define PRODUCT_NAME_VERSION PRODUCT_NAME " " VERSION_STR
