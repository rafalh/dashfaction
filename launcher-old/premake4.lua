project "DashFactionLauncher_OLD"
	kind "ConsoleApp"
	language "C++"
	files {"**.h", "**.c", "**.cpp", "*.rc"}
	links {"Shlwapi", "WinInet"}
	includedirs {
		"../common/include"
	}
	
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"
	
	configuration "Debug"
		targetdir "../bin/debug"
		debugdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
		debugdir "../bin/release"
