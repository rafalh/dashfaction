project "subhook"
	kind "StaticLib"
	files {
		"subhook.c",
		"subhook.h",
		"subhook_private.h",
	}
	defines {
		"SUBHOOK_STATIC",
	}
	includedirs {
		".",
	}
	
	configuration "Release"
		targetdir "lib/release"
	
	configuration "Debug"
		targetdir "lib/debug"
