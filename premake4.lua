-- Note: Premake 5 is required!
workspace "DashFaction"
	configurations { "Debug", "Release" }
	defines {
		"_CRT_SECURE_NO_WARNINGS",
		"_CRT_NONSTDC_NO_DEPRECATE",
		"_USE_MATH_DEFINES",
	}
	includedirs "include"
	language "C" -- default
	if location then
		location "premake-build"
	end
	
	characterset "MBCS"

	if os.isfile "main/pf.cpp" then
		defines { "HAS_PF" }
	end
	if os.isfile "main/experimental.cpp" then
		defines { "HAS_EXPERIMENTAL" }
	end
	
	configuration "Debug"
		defines { "DEBUG" }
		symbols "On"
	
	configuration "Release"
		defines { "NDEBUG" }
		optimize "Size"

	configuration "linux"
		defines { "LINUX" }
	
	configuration "vs*"
		toolset "v141_xp"
		buildoptions {
			"/Zc:threadSafeInit-",
			"/arch:IA32",
		}

	configuration "linux"
		gccprefix "i686-w64-mingw32-"
		cppdialect "c++17"
		linkoptions "-static"
	
	include "vendor"
	include "launcher-old"
	if os.host() == "windows" then
		include "launcher"
	end
	include "common"
	include "main"
	include "editor_mod"
	include "mod_common"
	include "logger"
	include "crash_handler"
