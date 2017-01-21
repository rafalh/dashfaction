project "DashFactionLauncher"
	kind "WindowedApp"
	language "C++"
	files {"**.h", "**.c", "**.cpp", "**.rc"}
	links {"Shlwapi", "WinInet", "psapi", "Common"}
	flags { "MFC", "WinMain" }
	
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"
	
	includedirs {
		"../vendor/d3d8",
		"../common/include",
	}
	
	configuration "Debug"
		targetdir "../bin/debug"
		debugdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
		debugdir "../bin/release"
