workspace "Gauntlet"
architecture "x64"
startproject "Sandbox"

configurations { "Debug", "Release"}

VULKAN_PATH = os.getenv("VULKAN_SDK")
local CURRENT_WORKING_DIRECTORY = os.getcwd()

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Gauntlet/vendor/GLFW/include"
IncludeDir["VULKAN"] = "%{VULKAN_PATH}"
IncludeDir["ImGui"] = "Gauntlet/vendor/imgui"
IncludeDir["vma"] =  "Gauntlet/vendor/vma/include"
IncludeDir["stb"] =  "Gauntlet/vendor/stb"
IncludeDir["glm"] =  "Gauntlet/vendor/glm"
IncludeDir["assimp"] = "Gauntlet/vendor/assimp"

group "Dependencies"
    include "Gauntlet/vendor/GLFW"
    include "Gauntlet/vendor/imgui"
    include "Gauntlet/vendor/vma"
    include "Gauntlet/vendor/assimp"
group ""

project "Gauntlet"
    location "Gauntlet"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "on"

    targetdir("Binaries/" .. outputdir .. "/%{prj.name}")
    objdir("Intermediate/" .. outputdir .. "/%{prj.name}")

    pchheader("GauntletPCH.h")
    pchsource("Gauntlet/Source/GauntletPCH.cpp")

    flags {"MultiProcessorCompile"}

    files 
    {
        "%{IncludeDir.glm}/**.hpp",
        "%{IncludeDir.glm}/**.inl",
        "%{prj.name}/Source/**.h","%{prj.name}/Source/**.cpp"
    }

    includedirs
    {
        "%{prj.name}/vendor",
        "%{prj.name}/Source",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb}",
        "%{IncludeDir.VULKAN}/Include",
        "%{IncludeDir.vma}/Include",
        "%{IncludeDir.assimp}/Include"
    }

    links
    {
        "GLFW",
        "ImGui",
        "VulkanMemoryAllocator",
        "assimp"
    }

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"GLFW_INCLUDE_NONE",
			"_CRT_SECURE_NO_WARNINGS",
			"GLM_FORCE_RADIANS",
			"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		    "ASSETS_PATH=\"".. CURRENT_WORKING_DIRECTORY .. "/Assets/\""
		}

    filter "configurations:Debug"
        defines "GNT_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "GNT_RELEASE"
        optimize "On"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++latest"
    staticruntime "on"
    
    targetdir("Binaries/" .. outputdir .. "/%{prj.name}")
    objdir("Intermediate/" .. outputdir .. "/%{prj.name}")

    files 
    {
        "%{prj.name}/Source/**.h","%{prj.name}/Source/**.cpp"
    }

    includedirs
    {
        "Sandbox/Source",
        "%{IncludeDir.glm}",
		"Gauntlet/vendor",
		"Gauntlet/Source"
    }

    links 
    {
		"Gauntlet"
    }


	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"GLM_FORCE_RADIANS",
			"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		    "ASSETS_PATH=\"".. CURRENT_WORKING_DIRECTORY .. "/Assets/\""
		}

    filter "configurations:Debug"
        defines "GNT_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "GNT_RELEASE"
        optimize "On"
        
project "Forge"
    location "Forge"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++latest"
    staticruntime "on"
    
    targetdir("Binaries/" .. outputdir .. "/%{prj.name}")
    objdir("Intermediate/" .. outputdir .. "/%{prj.name}")

    files 
    {
        "%{prj.name}/Source/**.h","%{prj.name}/Source/**.cpp"
    }

    includedirs
    {
        "Forge/Source",
        "%{IncludeDir.glm}",
		"Gauntlet/vendor",
		"Gauntlet/Source"
    }

    links 
    {
		"Gauntlet"
    }


	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"GLM_FORCE_RADIANS",
			"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		    "ASSETS_PATH=\"".. CURRENT_WORKING_DIRECTORY .. "/Assets/\""
		}

    filter "configurations:Debug"
        defines "GNT_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "GNT_RELEASE"
        optimize "On"