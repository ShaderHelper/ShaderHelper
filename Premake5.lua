include "Premake/Custom.lua"

external_ue = "External/UE"
external_d3d12memallocator = "External/D3D12MemoryAllocator"
external_d3dx12 = "External/D3DX12"
external_dxcompiler = "External/DXC"
external_agilitysdk = "External/AgilitySDK"

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

include(external_ue)
include(external_d3d12memallocator)
include(external_d3dx12)
include(external_dxcompiler)
include(external_agilitysdk)

include(project_shaderHelper)
include(project_frameWork)
group("Tests")
    include(project_unitTestFrameWork)
