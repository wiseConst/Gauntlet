workspace "Gauntlet" -- Solution
architecture "x64"
startproject "Forge"
flags { "MultiProcessorCompile" }
configurations { "Debug", "Release", "RelWithDebInfo"}

VULKAN_PATH = os.getenv("VULKAN_SDK")
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["VULKAN"] =   "%{VULKAN_PATH}"
IncludeDir["GLFW"] =     "Gauntlet/vendor/GLFW/include"
IncludeDir["ImGui"] =    "Gauntlet/vendor/imgui"
IncludeDir["vma"] =      "Gauntlet/vendor/vma/include"
IncludeDir["stb"] =      "Gauntlet/vendor/stb"
IncludeDir["glm"] =      "Gauntlet/vendor/glm"
IncludeDir["assimp"] =   "Gauntlet/vendor/assimp"
IncludeDir["entt"] =     "Gauntlet/vendor/entt/single_include"
IncludeDir["json"] =     "Gauntlet/vendor/json/single_include" 

Binaries = {}
Binaries["Assimp_Debug"] =          "%{wks.location}/Gauntlet/vendor/assimp/Binaries/Debug/assimp-vc143-mtd.dll"
Binaries["Assimp_Release"] =        "%{wks.location}/Gauntlet/vendor/assimp/Binaries/Release/assimp-vc143-mt.dll"
Binaries["Assimp_RelWithDebInfo"] = "%{wks.location}/Gauntlet/vendor/assimp/Binaries/Release/assimp-vc143-mt.dll"

Libraries = {}
Libraries["Assimp_Debug"] =          "%{wks.location}/Gauntlet/vendor/assimp/Binaries/Debug/assimp-vc143-mtd.lib"
Libraries["Assimp_Release"] =        "%{wks.location}/Gauntlet/vendor/assimp/Binaries/Release/assimp-vc143-mt.lib"
Libraries["Assimp_RelWithDebInfo"] = "%{wks.location}/Gauntlet/vendor/assimp/Binaries/Release/assimp-vc143-mt.lib"

group "Dependencies"
    include "Gauntlet/vendor/GLFW"
    include "Gauntlet/vendor/imgui"
    include "Gauntlet/vendor/vma"
group ""

group "Engine"

project "Gauntlet"
    location "Gauntlet"
    kind "StaticLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "off"

    targetdir("Binaries/" .. outputdir .. "/%{prj.name}")
    objdir("Intermediate/" .. outputdir .. "/%{prj.name}")

    pchheader("GauntletPCH.h")
    pchsource("Gauntlet/Source/GauntletPCH.cpp")

    files 
    {
        "%{IncludeDir.glm}/**.hpp",
        "%{IncludeDir.glm}/**.inl",
        "%{prj.name}/Source/**.h",
        "%{prj.name}/Source/**.cpp"
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
        "%{IncludeDir.assimp}/Include",
        "%{IncludeDir.entt}",
        "%{IncludeDir.json}"
    }

    links
    {
        "GLFW",
        "ImGui",
        "VulkanMemoryAllocator"
    }

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"GLFW_INCLUDE_NONE",
			"_CRT_SECURE_NO_WARNINGS",
			"GLM_FORCE_RADIANS",
			"GLM_FORCE_DEPTH_ZERO_TO_ONE"
		}

    filter "configurations:Debug"
        defines "GNT_DEBUG"
        symbols "On"
        optimize "Off"
        
    filter "configurations:Release"
        defines "GNT_RELEASE"
        symbols "Off"
        optimize "Full"

    filter "configurations:RelWithDebInfo"
        defines "GNT_RELEASE"
        symbols "On"
        optimize "Debug"
group ""
        
group "Editor"      
project "Forge"
    location "Forge"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++latest"
    staticruntime "off"
    
    targetdir("Binaries/" .. outputdir .. "/%{prj.name}")
    objdir("Intermediate/" .. outputdir .. "/%{prj.name}")

    files 
    {
        "%{prj.name}/Source/**.h",
        "%{prj.name}/Source/**.cpp"
    }

    includedirs
    {
        "Forge/Source",
        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.json}",
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
			"GLM_FORCE_DEPTH_ZERO_TO_ONE"
		}

    filter "configurations:Debug"
        defines "GNT_DEBUG"
        symbols "On"
        optimize "Off"
        
        links
        {
            "%{Libraries.Assimp_Debug}"
        }

        postbuildcommands 
        {
         	' {COPY} "%{Binaries.Assimp_Debug}" "%{cfg.targetdir}" '
        }


    filter "configurations:Release"
        kind "WindowedApp"
        defines "GNT_RELEASE"
        symbols "Off"
        optimize "Full"
        
        links
        {
            "%{Libraries.Assimp_Release}"
        }

        postbuildcommands 
        {
         	' {COPY} "%{Binaries.Assimp_Release}" "%{cfg.targetdir}" '
        }
        
    filter "configurations:RelWithDebInfo"
        defines "GNT_RELEASE"
        symbols "On"
        optimize "Debug"

        links
        {
            "%{Libraries.Assimp_RelWithDebInfo}"
        }

        postbuildcommands 
        {
         	' {COPY} "%{Binaries.Assimp_RelWithDebInfo}" "%{cfg.targetdir}" '
        }
group ""

group "Runtime"
project "Gauntlet-Runtime"
    location "Gauntlet-Runtime"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++latest"
    staticruntime "off"
    
    targetdir("Binaries/" .. outputdir .. "/%{prj.name}")
    objdir("Intermediate/" .. outputdir .. "/%{prj.name}")

    files 
    {
        "%{prj.name}/Source/**.h",
        "%{prj.name}/Source/**.cpp"
    }

    includedirs
    {
        "Gauntlet-Runtime/Source",
        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.json}",
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
			"GLM_FORCE_DEPTH_ZERO_TO_ONE"
		}

    filter "configurations:Debug"
        defines "GNT_DEBUG"
        symbols "On"
        optimize "Off"
        
        links
        {
            "%{Libraries.Assimp_Debug}"
        }

        postbuildcommands 
        {
         	' {COPY} "%{Binaries.Assimp_Debug}" "%{cfg.targetdir}" '
        }


    filter "configurations:Release"
        kind "WindowedApp"
        defines "GNT_RELEASE"
        symbols "Off"
        optimize "Full"
        
        links
        {
            "%{Libraries.Assimp_Release}"
        }

        postbuildcommands 
        {
         	' {COPY} "%{Binaries.Assimp_Release}" "%{cfg.targetdir}" '
        }
        
    filter "configurations:RelWithDebInfo"
        defines "GNT_RELEASE"
        symbols "On"
        optimize "Debug"

        links
        {
            "%{Libraries.Assimp_RelWithDebInfo}"
        }

        postbuildcommands 
        {
         	' {COPY} "%{Binaries.Assimp_RelWithDebInfo}" "%{cfg.targetdir}" '
        }
group ""