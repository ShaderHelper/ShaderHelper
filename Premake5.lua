include "Premake/custom.lua"

external_ue = "External/UE"
external_dxcompiler = "External/DXC"
external_agilitysdk = "External/AgilitySDK"
external_pix = "External/WinPixEventRuntime"
external_mtlpp = "External/MtlppUE"
external_shaderConductor = "External/ShaderConductor"
external_magicenum = "External/magic_enum"

project_shaderHelper = "Source/ShaderHelper"
project_frameWork = "Source/FrameWork"
project_unitTestUtil = "Source/Tests/UnitTestUtil"
project_unitTestGpuApi = "Source/Tests/UnitTestGpuApi"

newoption {
    trigger = "UniversalBinary",
    description = "Build universal app that supports arm64 and x86",
}

workspace "ShaderHelper"    
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
        architecture "x86_64"  
        targetname "%{prj.name}-%{cfg.buildcfg}"
        systemversion "latest"
        runtime "Release"
        targetdir ("Binaries/Win64")
        implibdir ("%{cfg.objdir}/%{cfg.buildcfg}")
        files {
            "External/UE/Unreal.natvis",
        }

        -- workspace_items
        -- {
            -- ["UE"] = {"External/UE/Src"},
        -- }
        
        vpaths {
            ["Visualizers"] = { "External/UE/Unreal.natvis"},
        }
        buildoptions 
        { 
            "/utf-8",
            "/Zc:__cplusplus",
			"/Zc:hiddenFriend",
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
        architecture "universal"
        --Premake can not link the corresponding "SharedLib" project in Xcode when targetname contains build configuration.
        targetname "%{prj.name}"
        targetdir ("Binaries/Mac")
        xcodebuildsettings { 
            ["MACOSX_DEPLOYMENT_TARGET"] = "10.15",
            --This must be set to YES on arm64 mac.
            ["ENABLE_STRICT_OBJC_MSGSEND"] = "YES",
        }
        runpathdirs {
            "%{cfg.targetdir}",
            "%{cfg.targetdir}/../../../ "
        }
        buildoptions 
        { 
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",  
        }
        if _OPTIONS["UniversalBinary"] then
            xcodebuildsettings {
                ["ARCHS"] = "x86_64 arm64",
                ["ONLY_ACTIVE_ARCH"] = "No",
            }
        end

    filter {"system:macosx","kind:SharedLib"}
        xcodebuildsettings { ["DYLIB_INSTALL_NAME_BASE"] = "@rpath" }

    filter {"system:macosx","files:**/Win/*.cpp"}
        flags {"ExcludeFromBuild"}

    filter {"system:windows","files:**/Mac/*.cpp or **.mm or **.hlsl"}
        flags {"ExcludeFromBuild"}
	

    filter {"configurations:Debug"}
        optimize "Off"
    
    filter {"configurations:Dev"}
		optimize "On"

include(external_ue)
include(external_dxcompiler)
include(external_agilitysdk)
include(external_pix)
include(external_mtlpp)
include(external_shaderConductor)
include(external_magicenum)

include(project_shaderHelper)
include(project_frameWork)
group("Tests")
    include(project_unitTestUtil)
	include(project_unitTestGpuApi)
