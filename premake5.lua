include "Premake/custom.lua"

external_ue = "External/UE"
external_dxcompiler = "External/DXC"
external_agilitysdk = "External/AgilitySDK"
external_pix = "External/WinPixEventRuntime"
external_metalcpp = "External/metal-cpp"
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
        "Shipping" --RelWithDebInfo
    } 
    targetname "%{prj.name}"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    exceptionhandling "off"

    objdir ("Intermediate/%{prj.name}")

    symbols "On"

    startproject "ShaderHelper"

    filter "system:windows"
        architecture "x86_64"  
        systemversion "latest"
        runtime "Release"
        targetdir ("Binaries/Win64")
        implibdir ("%{cfg.objdir}/%{cfg.buildcfg}")

        workspace_items
        {
            "Resource/Shaders",
            ["Visualizers"] = { "External/UE/Unreal.natvis"},
        }

        buildoptions 
        { 
            "/utf-8",
            "/Zc:__cplusplus",
			
			--https://devblogs.microsoft.com/cppblog/improving-the-state-of-debug-performance-in-c/
			"/permissive-", --implicitly contains "/Zc:hiddenFriend" "/Zc:strictStrings"
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
        targetdir ("Binaries/Mac")
        xcodebuildsettings { 
            ["MACOSX_DEPLOYMENT_TARGET"] = "10.15",
            --This must be set to YES on arm64 mac.
            ["ENABLE_STRICT_OBJC_MSGSEND"] = "YES",
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

    filter "system:linux"
        architecture "x86_64"
        targetdir ("Binaries/Linux")
        runpathdirs {
            "%{cfg.targetdir}",
            "%{cfg.targetdir}/../../../ "
        }
        buildoptions 
        { 
            "-fvisibility=hidden",
            "-fvisibility-inlines-hidden",  
        }

    filter {"system:macosx","kind:WindowedApp"}
        runpathdirs {
            "%{cfg.targetdir}/../../../"
        }

    filter {"system:macosx","kind:SharedLib"}
        xcodebuildsettings { ["DYLIB_INSTALL_NAME_BASE"] = "@rpath" }

    filter {"system:macosx","files:**/Linux/*.cpp or **/Win/*.cpp"}
        flags {"ExcludeFromBuild"}

    filter {"system:windows","files:**/Linux/*.cpp or **/Mac/*.cpp or **.mm or **.hlsl"}
        flags {"ExcludeFromBuild"}

    filter {"system:linux","files:**/Win/*.cpp or **/Mac/*.cpp or **.mm or **.hlsl"}
        flags {"ExcludeFromBuild"}
    
    filter {"system:linux"}
        toolset "clang"

    filter {"configurations:Debug"}
        optimize "Off"
    
    filter {"configurations:Shipping"}
		optimize "On"
		defines "SH_SHIPPING=1"

    --Premake can not link the corresponding "SharedLib" project in Xcode when targetname contains build configuration.
    filter {"system:windows or linux", "configurations:not Shipping"}
        targetsuffix "-%{cfg.buildcfg}"

include(external_ue)
include(external_dxcompiler)
include(external_agilitysdk)
include(external_pix)
include(external_metalcpp)
include(external_shaderConductor)
include(external_magicenum)

include(project_shaderHelper)
include(project_frameWork)
group("Tests")
    include(project_unitTestUtil)
	include(project_unitTestGpuApi)
