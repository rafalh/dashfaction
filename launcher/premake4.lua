project "DashFactionLauncher"
	kind "ConsoleApp"
	language "C"
	files {"**.h", "**.c"}
	links {"Shlwapi"}
	
	configuration "Debug"
		targetdir "../bin/debug"
		debugdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
		debugdir "../bin/release"
