project "ModCommon"
	kind "StaticLib"
	language "C++"
	files { "**.h", "**.c", "**.cpp" }
	includedirs {
		".",
		"../vendor/subhook",
		"../logger/include",
	}
