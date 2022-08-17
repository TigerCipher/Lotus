project "Lotus"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("%{wks.location}/bin/" .. outputdir)
	objdir ("%{wks.location}/obj/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "src/pch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
	}

	includedirs
	{
		"src",
	}

	links
	{

	}


	defines {
		"_CRT_SECURE_NO_WARNINGS",
	}

	filter "system:windows"
		systemversion "latest"


	filter "configurations:Debug"
		defines
		{
			"L_DEBUG"
		}
		symbols "on"
		runtime "Debug"


	filter "configurations:Release"
		defines "L_RELEASE"
		optimize "on"
		runtime "Release"
