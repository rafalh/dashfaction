project "DashFaction"
	kind "SharedLib"
	language "C++"
	files { "**.h", "**.c", "**.cpp" }
	defines {"BUILD_DLL", "SUBHOOK_STATIC"}
	includedirs {
		"../mod_common",
		"../logger/include",
		"../vendor",
		"../vendor/unzip",
		"../vendor/zlib",
		"../vendor/unrar",
		"../vendor/d3d8",
		"../vendor/subhook",
	}
	links { "psapi", "wininet", "unrar", "unzip", "zlib", "ModCommon", "subhook", "logger" }
	
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"
	
	configuration "Debug"
		targetdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
