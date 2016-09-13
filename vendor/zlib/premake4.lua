project "zlib"
	kind "StaticLib"
	language "C"
	files {
		"adler32.c",
		"crc32.c",
		"inffast.c",
		"inflate.c",
		"inftrees.c",
		"zutil.c"
	}
