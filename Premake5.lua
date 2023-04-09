include "Premake/Custom.lua"

external_ue = "External/UE"
external_d3d12memallocator = "External/D3D12MemoryAllocator"
external_dxcompiler = "External/DXC"
external_agilitysdk = "External/AgilitySDK"
external_pix = "External/WinPixEventRuntime"
external_mtlpp = "External/MtlppUE"

project_shaderHelper = "Source/ShaderHelper"
project_frameWork = "Source/FrameWork"
project_unitTestFrameWork = "Source/Tests/UnitTestFrameWork"

workspace "ShaderHelper"  
    architecture "x64"    
    configurations 
    { 
        "Debug",
        "Dev"
    } 

    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    exceptionhandling "off"

    objdir ("Intermediate/%{prj.name}")

    symbols "On"

    startproject "ShaderHelper"

    filter "system:windows"
        targetname "%{prj.name}-%{cfg.buildcfg}"
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
            "/Zc:__cplusplus",
        }
        linkoptions 
        { 
            "/NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib",
        }
        defines 
        {
            "NDEBUG",
        }

    filter "system:macosx"
        --Premake can not link the corresponding "SharedLib" project in Xcode when targetname contains build configuration.
        targetname "%{prj.name}"
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

    filter {"system:windows","files:**.mm"}
        flags {"ExcludeFromBuild"}

    filter {"configurations:Debug"}
        optimize "Off"
    
    filter {"configurations:Dev"}
		optimize "On"

include(external_ue)
include(external_d3d12memallocator)
include(external_dxcompiler)
include(external_agilitysdk)
include(external_pix)
include(external_mtlpp)

include(project_shaderHelper)
include(project_frameWork)
group("Tests")
    include(project_unitTestFrameWork)
