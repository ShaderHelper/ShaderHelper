workspace "ShaderHelper"  
    architecture "x64"
    configurations 
    { 
        "Debug",
        "Dev"
    } 

UE_BaseIncludeDir = {
    Core = "External/UE/Include/Core",
    TraceLog = "External/UE/Include/TraceLog",
    ApplicationCore = "External/UE/Include/ApplicationCore",
    Slate = "External/UE/Include/Slate",
    InputCore = "External/UE/Include/InputCore",
    StandaloneRenderer = "External/UE/Include/StandaloneRenderer",
    CoreUObject = "External/UE/Include/CoreUObject",
    SlateCore = "External/UE/Include/SlateCore",
    Json = "External/UE/Include/Json",
    ImageWrapper = "External/UE/Include/ImageWrapper",
}

UE_UHTIncludeDir = {
    InputCore = "External/UE/Include/InputCore/UHT",
    Slate = "External/UE/Include/Slate/UHT",
    CoreUObject = "External/UE/Include/CoreUObject/UHT",
    SlateCore = "External/UE/Include/SlateCore/UHT",
}

ShaderHelperHierarchy = {
    ["Sources/*"] = {"Source/**.h","Source/**.cpp"},
    ["External/UE"] = { "External/UE/Include/SharedPCH.h" },
}

project "ShaderHelper"
    kind "WindowedApp"   
    targetname "ShaderHelper-%{cfg.buildcfg}"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    exceptionhandling "off"

    objdir ("Intermediate")

    vpaths(ShaderHelperHierarchy)

    files 
    { 
        "Source/**.h", 
        "Source/**.cpp",
        "External/UE/Include/SharedPCH.h",
    }
    
    includedirs
    {
        "Source",
    }

    externalincludedirs
    {
        "External/UE/Include",
        "%{UE_BaseIncludeDir.Core}",
        "%{UE_BaseIncludeDir.TraceLog}",
        "%{UE_BaseIncludeDir.ApplicationCore}",
        "%{UE_BaseIncludeDir.Slate}",
        "%{UE_BaseIncludeDir.InputCore}",
        "%{UE_BaseIncludeDir.StandaloneRenderer}",
        "%{UE_BaseIncludeDir.CoreUObject}",
        "%{UE_BaseIncludeDir.SlateCore}",
        "%{UE_BaseIncludeDir.Json}",
        "%{UE_BaseIncludeDir.ImageWrapper}",

        "%{UE_UHTIncludeDir.InputCore}",
        "%{UE_UHTIncludeDir.Slate}",
        "%{UE_UHTIncludeDir.CoreUObject}",
        "%{UE_UHTIncludeDir.SlateCore}",
    }
  

    filter "system:windows"
        systemversion "latest"
        runtime "Release"
        symbols "On"
        targetdir ("Binaries/Win64")
        pchheader "CommonHeader.h"
        pchsource "Source/CommonHeader.cpp"
        buildoptions 
        { 
            "/utf-8",
            "/GR-",  
        }
        removefiles 
        {
            "Source/FrameWork/GpuApi/Mac/**.cpp",
            "Source/FrameWork/GpuApi/Mac/**.h",
            "Source/Launch/MacMain.cpp",
        }
        files 
        {
            "External/UE/Unreal.natvis",
            "Resource/ExeIcon/Windows/*",
            "External/UE/Include/UnrealDefinitionsWin.h",
        }
        vpaths
        {
            ["Resource/*"] = {"Resource/ExeIcon/Windows/*"},
            ["External/UE/Visualizers"] = { "External/UE/Unreal.natvis"},
            ["External/UE"] = { "External/UE/Include/UnrealDefinitionsWin.h" },
        }
        libdirs
        {
            "External/UE/Lib",
        }
        defines 
        {
            "NDEBUG",
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
        targetdir ("Binaries/Mac")
        --pchheader "Source/CommonHeader.h"
        removefiles 
        {
            "Source/FrameWork/GpuApi/Win/**.cpp",
            "Source/FrameWork/GpuApi/Win/**.h",
            "Source/Launch/WinMain.cpp",
        }
        files
        {
            "External/UE/Include/UnrealDefinitionsMac.h",
            "Resource/ExeIcon/Mac/*",
        }
        vpaths
        {
            ["External/UE"] = { 
                "External/UE/Include/UnrealDefinitionsMac.h",
            },
        }
        links {
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
        runpathdirs {
            "%{cfg.targetdir}",
            "%{cfg.targetdir}/../../../ "
        }
        buildoptions { 
            "-x objective-c++",
            "-fno-rtti", --UE modules disable rtti
        }
        xcodebuildsettings { ["MACOSX_DEPLOYMENT_TARGET"] = "10.15" }

        
    filter {"configurations:Debug"}
        optimize "Off"
        

    filter {"configurations:Dev"}
        symbols "On"
