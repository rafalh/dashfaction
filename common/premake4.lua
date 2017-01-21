project "Common"
	kind "StaticLib"
	language "C++"
	files { "**.h", "**.cpp", "../include/**.h" }
	includedirs {
		".",
		"include",
		"../vendor/d3d8",
	}
