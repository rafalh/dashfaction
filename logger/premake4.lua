project "Logger"
	kind "StaticLib"
	language "C++"
	files {"**.h", "**.cpp"}
	
	configuration "Debug"
		targetdir "lib/debug"
	
	configuration "Release"
		targetdir "lib/release"
