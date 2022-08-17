include "vendor/premake/solution_items.lua"
include "Dependencies.lua"

workspace "Lotus"
	architecture "x86_64"
	startproject "LotusEditor"


	configurations
	{
		"Debug",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	solution_items
	{
		--".editorconfig",
		".gitignore",
		"README.md"
	}

outputdir = "%{cfg.buildcfg}"

group "Utilities"
	include "vendor/premake"
	--include "scripts"
group ""

-- group "Dependencies"
-- 	include "vendor/deps/libname"
-- group ""

include "Lotus"

externalproject "LotusEditor"
    location "LotusEditor"
    uuid "9AEE8D99-BDC1-4AB6-95E6-349C6A780E22"
    kind "WindowedApp"
    language "C#"