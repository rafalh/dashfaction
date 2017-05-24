project "CrashHandler"
	kind "WindowedApp"
	language "C++"
	files {"**.h", "**.c", "**.cpp", "*.rc"}
	links {
		"psapi",
	}
	includedirs {
		"../common/include",
	}
	
	configuration "Debug"
		targetdir "../bin/debug"
		debugdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
		debugdir "../bin/release"
