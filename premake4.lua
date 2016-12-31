(workspace or solution) "DashFaction"
	configurations { "Debug", "Release" }
	defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE" }
	includedirs "include"
	language "C" -- default
	if location then
		location "premake-build"
	end
	if toolset then
		toolset "v140_xp"
	end
	if characterset then
		characterset "MBCS"
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
	
	include "vendor"
	include "launcher"
	include "main"
	include "mod_common"
	include "logger"
