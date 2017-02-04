project "DashFaction"
	kind "SharedLib"
	language "C++"
	files { "**.h", "**.c", "**.cpp" }
	defines {
		"NOMINMAX",
		"BUILD_DLL",
		"SUBHOOK_STATIC",
	}
	includedirs {
		"../mod_common",
		"../logger/include",
		"../common/include",
		"../vendor",
		"../vendor/unzip",
		"../vendor/zlib",
		"../vendor/unrar",
		"../vendor/d3d8",
		"../vendor/subhook",
	}
	links { "psapi", "wininet", "version", "unrar", "unzip", "zlib", "ModCommon", "subhook", "logger", "Common" }
	
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"
	
	configuration "Debug"
		targetdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
