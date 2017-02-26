project "DashEditor"
	kind "SharedLib"
	language "C++"
	files { "**.h", "**.c", "**.cpp" }
	defines {
		"NOMINMAX",
		"BUILD_DLL",
		"SUBHOOK_STATIC",
	}
	includedirs {
		"../mod_common",
		"../common/include",
	}
	links { "ModCommon", "Common" }
	
	configuration "vs*"
		linkoptions { "/DEBUG" } -- generate PDB files
	
	configuration "Debug"
		targetdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"
