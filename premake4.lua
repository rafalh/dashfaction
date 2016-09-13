solution "DashFaction"
	configurations { "Debug", "Release" }
	defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_DEPRECATE" }
	includedirs "include"
	language "C" -- default
	
	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols" }
	
	configuration "Release"
		defines { "NDEBUG" }
		flags { "OptimizeSize" }

	configuration "linux"
		defines { "LINUX" }
	
	include "vendor"
	include "launcher"
	include "main"
