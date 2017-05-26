project "unzip"
	kind "StaticLib"
	language "C"
	files {
		"unzip.c", "unzip.h",
		--"iowin32.c", "iowin32.h",
		"ioapi.c", "ioapi.h",
	}
	includedirs {
		"../.."
	}
	
project "zip"
	kind "StaticLib"
	language "C"
	files {
		"zip.c", "zip.h",
		--"iowin32.c", "iowin32.h",
		"ioapi.c", "ioapi.h",
	}
	includedirs {
		"../.."
	}
