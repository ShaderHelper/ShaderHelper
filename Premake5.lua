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
	["External/UE"] = { "External/UE/Include/Definitions.h" },
    ["External/UE/Visualizers"] = { "External/UE/Unreal.natvis"},
}

project "ShaderHelper"
	targetname "ShaderHelper-%{cfg.buildcfg}"
    kind "WindowedApp"   
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("Binaries")
    objdir ("Intermediate")

    pchheader "CommonHeader.h"
	pchsource "Source/CommonHeader.cpp"
	
    vpaths(ShaderHelperHierarchy)

    files 
    { 
        "Source/**.h", 
        "Source/**.cpp",
		"External/UE/Unreal.natvis",
		"External/UE/Include/Definitions.h"
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

    libdirs
    {
        "External/UE/Lib",
    }

    links
    {
        "UE-Core.lib",
        "UE-ApplicationCore.lib",
        "UE-Slate.lib",
        "UE-SlateCore.lib",
        "UE-StandaloneRenderer.lib",
        "UE-Projects.lib",
        "UE-ImageWrapper.lib",
        "UE-CoreUObject.lib"
    }
    
  

    filter "system:windows"
		systemversion "latest"
        buildoptions 
        { 
            "/utf-8",
			"/FI\"$(ProjectDir)/External/UE/Include/Definitions.h\"",
        }
		
	filter "configurations:Debug"  
		defines 
		{
			"NDEBUG",
			"_WINDOWS"
		}
		runtime "Release"    
		optimize "Off"
		symbols "On"

    filter "configurations:Dev"  
        defines 
		{
			"NDEBUG",
			"_WINDOWS"
		}
        runtime "Release"    
        optimize "On"
        symbols "On"