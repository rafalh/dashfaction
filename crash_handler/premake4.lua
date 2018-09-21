project "CrashHandler"
	kind "WindowedApp"
	language "C++"
	files {"**.h", "**.c", "**.cpp", "*.rc"}
	links {
		"psapi",
		"zip",
		"zlib",
		"wininet",
	}
	includedirs {
		"../common/include",
		"../vendor/zlib",
		"../vendor/zlib/contrib/minizip",
	}
	-- override extension for cross-compilation
	targetextension ".exe"
	
	configuration "Debug"
		targetdir "../bin/debug"
		if debugdir then
			debugdir "../bin/debug"
		end
	
	configuration "Release"
		targetdir "../bin/release"
		if debugdir then
			debugdir "../bin/release"
		end
