project "DashFaction"
	kind "SharedLib"
	language "C++"
	files { "**.h", "**.c", "**.cpp", "*.rc" }
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
		"../vendor/xxhash",
	}
	links {
		"psapi",
		"wininet",
		"version",
		"shlwapi",
		"unrar",
		"unzip",
		"zlib",
		"ModCommon",
		"subhook",
		"logger",
		"Common",
		"xxhash",
	}
	
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"
	
	configuration "vs*"
		linkoptions { "/DEBUG" } -- generate PDB files
	
	configuration "Debug"
		targetdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
