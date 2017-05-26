project "CrashHandler"
	kind "WindowedApp"
	language "C++"
	files {"**.h", "**.c", "**.cpp", "*.rc"}
	links {
		"psapi",
		"zip",
		"zlib",
		"WinINet",
	}
	includedirs {
		"../common/include",
		"../vendor/zlib",
		"../vendor/zlib/contrib/minizip",
	}
	
	configuration "Debug"
		targetdir "../bin/debug"
		debugdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
		debugdir "../bin/release"
