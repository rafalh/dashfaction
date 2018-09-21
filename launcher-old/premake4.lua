project "DashFactionLauncher_OLD"
	kind "ConsoleApp"
	language "C++"
	files {"**.h", "**.c", "**.cpp", "*.rc"}
	links {"shlwapi", "wininet"}
	includedirs {
		"../common/include"
	}
	
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"
	
	-- override extension for cross-compilation
	targetextension ".exe"

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
