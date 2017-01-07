project "DashFactionLauncher"
	kind "ConsoleApp"
	language "C++"
	files {"**.h", "**.c", "**.cpp"}
	links {"Shlwapi", "WinInet"}
	
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"
	
	configuration "Debug"
		targetdir "../bin/debug"
		debugdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
		debugdir "../bin/release"
