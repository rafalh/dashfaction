project "unzip"
	kind "StaticLib"
	language "C"
	files { "**.h", "**.c" }
	includedirs { "../zlib" }
	links { "zlib" }

