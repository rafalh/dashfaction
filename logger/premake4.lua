project "Logger"
	kind "StaticLib"
	language "C++"
	files {"include/**.h", "src/**.cpp"}
	
	includedirs "include"
	
	configuration "Debug"
		targetdir "lib/debug"
	
	configuration "Release"
		targetdir "lib/release"
