project "DashFactionLauncher"
	kind "WindowedApp"
	language "C++"
	files { "**.h", "**.c", "**.cpp", "**.rc", "res/*.manifest" }
	links { "Shlwapi", "WinInet", "psapi", "Common" }
	flags { "MFC", "WinMain" }
	
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"
	
	includedirs {
		"../vendor/d3d8",
		"../common/include",
	}
	
	configuration "vs*"
		linkoptions { "/DEBUG" } -- generate PDB files
	
	configuration "Debug"
		targetdir "../bin/debug"
		if debugdir then
			debugdir "../bin/debug"
		end
	
	configuration "Release"
		targetdir "../bin/release"
		if debugdir then
			debugdir "../bin/release"
		end
