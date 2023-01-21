include "Premake/Custom.lua"

workspace "ShaderHelper"  
    architecture "x64"    
    configurations 
    { 
        "Debug",
        "Dev"
    } 

    targetname "%{prj.name}-%{cfg.buildcfg}"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    exceptionhandling "off"

    objdir ("Intermediate/%{prj.name}")

    symbols "On"

    startproject "ShaderHelper"

    filter "system:windows"
        systemversion "latest"
        runtime "Release"
        targetdir ("Binaries/Win64")
        implibdir ("%{cfg.objdir}/%{cfg.buildcfg}")
        files {
            "External/UE/Unreal.natvis",
        }
        vpaths {
            ["Visualizers"] = { "External/UE/Unreal.natvis"},
        }
        buildoptions 
        { 
            "/utf-8",
            "/GR-",  
        }
        defines 
        {
            "NDEBUG",
        }

    filter "system:macosx"
        targetdir ("Binaries/Mac")
        xcodebuildsettings { ["MACOSX_DEPLOYMENT_TARGET"] = "10.15" }
        runpathdirs {
            "%{cfg.targetdir}",
            "%{cfg.targetdir}/../../../ "
        }
        buildoptions 
        { 
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",  
        }

    filter {"system:macosx","kind:SharedLib"}
        xcodebuildsettings { ["DYLIB_INSTALL_NAME_BASE"] = "@rpath" }

    filter {"system:macosx","files:**/Win/*.cpp"}
        flags {"ExcludeFromBuild"}

    filter {"system:windows","files:**/Mac/*.cpp"}
        flags {"ExcludeFromBuild"}

    filter {"configurations:Debug"}
        optimize "Off"
    
    filter {"configurations:Dev"}
		optimize "On"

usage "UE"
    GenerateIncludeDir_UE()
    filter "system:windows"
        libdirs
        {
            "External/UE/Lib",
        }
        links
        {
            "UE-Core",
            "UE-ApplicationCore",
            "UE-Slate",
            "UE-SlateCore",
            "UE-StandaloneRenderer",
            "UE-Projects",
            "UE-ImageWrapper",
            "UE-CoreUObject"
        }
    filter "system:macosx"
        links 
        {
            "Cocoa.framework",
            "Metal.framework",
            "OpenGL.framework",
            "Carbon.framework",
            "IOKit.framework",
            "Security.framework",
            "GameController.framework",
            "QuartzCore.framework",
            "IOSurface.framework",
            "%{cfg.targetdir}/UE-Core.dylib",
            "%{cfg.targetdir}/UE-ApplicationCore.dylib",
            "%{cfg.targetdir}/UE-Slate.dylib",
            "%{cfg.targetdir}/UE-SlateCore.dylib",
            "%{cfg.targetdir}/UE-StandaloneRenderer.dylib",
            "%{cfg.targetdir}/UE-Projects.dylib",
            "%{cfg.targetdir}/UE-ImageWrapper.dylib",
            "%{cfg.targetdir}/UE-CoreUObject.dylib"
        }
        buildoptions { 
            "-x objective-c++",
            "-fno-rtti", --UE modules disable rtti
        }

project_ShaderHelper = "Source/ShaderHelper"
project_FrameWork = "Source/FrameWork"
project_UnitTestFrameWork = "Source/Tests/UnitTestFrameWork"

include(project_ShaderHelper)
include(project_FrameWork)
group("Tests")
include(project_UnitTestFrameWork)
