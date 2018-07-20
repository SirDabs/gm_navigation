solution "navigation"
	language		"C++"
	architecture	"x86"
	location		"project"
	targetdir		"bin"

	flags "StaticRuntime"

	local target_suffixes = {
		bsd       = "_linux",
		linux     = "_linux",
		solaris   = "_linux",
		windows   = "_win32",
		macosx    = "_osx"  ,
	}

	defines 	{ "GMMODULE", "RAD_TELEMETRY_DISABLED" }
	configurations { "Server", "Client" }

	configuration "Server"
		buildoptions {"-m32 -msse -std=gnu++0x"}
		optimize 	"On"
		targetprefix "gmsv_"
		defines "LUA_SERVER"
		
	configuration "Client"
		buildoptions {"-m32 -msse -std=gnu++0x"}
		optimize 	"On"
		targetprefix "gmcl_"

	project "navigation"
		targetextension		".dll"
		targetsuffix		( target_suffixes[os.get()] )
		if (os.get() ~= "windows") then
			defines { "NO_MALLOC_OVERRIDE", "_LINUX", "LINUX",
		"_POSIX", "POSIX", "GNUC"}
		else
			defines { "_WIN32", "WIN32" }
			characterset("MBCS")
		end
		kind	"SharedLib"
		includedirs{ 
			"../source-sdk-2013/mp/src/lib/public",
			"../source-sdk-2013/mp/src/lib/common",
			"../source-sdk-2013/mp/src/public",
			"../source-sdk-2013/mp/src/public/tier0",
			"../source-sdk-2013/mp/src/public/tier1",
			"../source-sdk-2013/mp/src/common",
			"../garrysmod_common/include",
			"../garrysmod_common/include/garrysmod/lua",
			"./",
			"../source-sdk-2013/mp/src/thirdparty/SDL2",
			"../source-sdk-2013/mp/src/public/mathlib"
		
		}
			
		files 	{ "navigation/sources/**.cpp","navigation/sources/**.c"}
		links{
		"../source-sdk-2013/mp/src/lib/public/linux32/mathlib",
		"../source-sdk-2013/mp/src/lib/public/linux32/tier0",
		"../source-sdk-2013/mp/src/lib/public/linux32/tier1",
		"../source-sdk-2013/mp/src/lib/public/linux32/vstdlib"}
		linkoptions { '' }
