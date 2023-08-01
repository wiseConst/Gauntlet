workspace "Eclipse"
architecture "x64"
startproject "Sandbox"

configurations { "Debug", "Release"}

VULKAN_PATH = os.getenv("VULKAN_SDK")
local CURRENT_WORKING_DIRECTORY = os.getcwd()

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Eclipse/vendor/GLFW/include"
IncludeDir["VULKAN"] = "%{VULKAN_PATH}"
IncludeDir["ImGui"] = "Eclipse/vendor/imgui"
IncludeDir["vma"] =  "Eclipse/vendor/vma/include"
IncludeDir["stb"] =  "Eclipse/vendor/stb"
IncludeDir["glm"] =  "Eclipse/vendor/glm"
IncludeDir["assimp"] = "Eclipse/vendor/assimp"

group "Dependencies"
    include "Eclipse/vendor/GLFW"
    include "Eclipse/vendor/imgui"
    include "Eclipse/vendor/vma"
    include "Eclipse/vendor/assimp"
group ""

project "Eclipse"
    location "Eclipse"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir("Binaries/" .. outputdir .. "/%{prj.name}")
    objdir("Intermediate/" .. outputdir .. "/%{prj.name}")

    pchheader("EclipsePCH.h")
    pchsource("Eclipse/Source/EclipsePCH.cpp")

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
        defines "ELS_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "ELS_RELEASE"
        optimize "On"

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
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
		"Eclipse/vendor",
		"Eclipse/Source"
    }

    links 
    {
		"Eclipse"
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
        defines "ELS_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "ELS_RELEASE"
        optimize "On"
        
project "EclipseEd"
    location "EclipseEd"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"
    
    targetdir("Binaries/" .. outputdir .. "/%{prj.name}")
    objdir("Intermediate/" .. outputdir .. "/%{prj.name}")

    files 
    {
        "%{prj.name}/Source/**.h","%{prj.name}/Source/**.cpp"
    }

    includedirs
    {
        "EclipseEd/Source",
        "%{IncludeDir.glm}",
		"Eclipse/vendor",
		"Eclipse/Source"
    }

    links 
    {
		"Eclipse"
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
        defines "ELS_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "ELS_RELEASE"
        optimize "On"