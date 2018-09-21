(workspace or solution) "DashFaction"
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
	
	if characterset then
		characterset "MBCS" -- Premake 5
	end

	if os.isfile "main/pf.cpp" then
		defines { "HAS_PF" }
	end
	
	configuration "Debug"
		defines { "DEBUG" }
		if symbols then symbols "On"
		else flags { "Symbols" } end
	
	configuration "Release"
		defines { "NDEBUG" }
		flags { "OptimizeSize" }

	configuration "linux"
		defines { "LINUX" }
	
	configuration "vs*"
		if toolset then toolset "v140_xp" end
		buildoptions {
			"/Zc:threadSafeInit-",
			"/arch:IA32",
		}

	configuration "linux"
		gccprefix "i686-w64-mingw32-"
		cppdialect "gnu++14"
		linkoptions "-static"
	
	include "vendor"
	include "launcher-old"
	if os.get() == "windows" then
		include "launcher"
	end
	include "common"
	include "main"
	include "editor_mod"
	include "mod_common"
	include "logger"
	include "crash_handler"
