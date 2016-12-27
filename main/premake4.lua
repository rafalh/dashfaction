project "DashFaction"
	kind "SharedLib"
	language "C++"
	files { "**.h", "**.c", "**.cpp" }
	defines {"BUILD_DLL"}
	includedirs {
		"../vendor",
		"../vendor/unzip",
		"../vendor/zlib",
		"../vendor/unrar",
		"../vendor/d3d8",
	}
	links { "psapi", "wininet", "unrar", "unzip", "zlib" }
	
	configuration "Debug"
		targetdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
