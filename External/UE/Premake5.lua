
ue_baseIncludeDir = {
    core = _WORKING_DIR .. "/External/UE/Include/Core",
    traceLog = _WORKING_DIR .. "/External/UE/Include/TraceLog",
    applicationCore = _WORKING_DIR .. "/External/UE/Include/ApplicationCore",
    slate = _WORKING_DIR .. "/External/UE/Include/Slate",
    inputCore = _WORKING_DIR .. "/External/UE/Include/InputCore",
    standaloneRenderer = _WORKING_DIR .. "/External/UE/Include/StandaloneRenderer",
    coreUObject = _WORKING_DIR .. "/External/UE/Include/CoreUObject",
    slateCore = _WORKING_DIR .. "/External/UE/Include/SlateCore",
    json = _WORKING_DIR .. "/External/UE/Include/Json",
    imageWrapper = _WORKING_DIR .. "/External/UE/Include/ImageWrapper",
	desktopPlatform = _WORKING_DIR .. "/External/UE/Include/DesktopPlatform",
	directoryWatcher = _WORKING_DIR .. "/External/UE/Include/DirectoryWatcher",
}

ue_uhtIncludeDir = {
    inputCore = _WORKING_DIR .. "/External/UE/Include/InputCore/UHT",
    slate = _WORKING_DIR .. "/External/UE/Include/Slate/UHT",
    coreUObject = _WORKING_DIR .. "/External/UE/Include/CoreUObject/UHT",
    slateCore = _WORKING_DIR .. "/External/UE/Include/SlateCore/UHT",
}

function generateIncludeDir_ue()
    externalincludedirs(_WORKING_DIR .. "/External/UE/Include")
    externalwarnings("off")

    for _, v in pairs(ue_baseIncludeDir) do
        externalincludedirs(v)
    end

    for _, v in pairs(ue_uhtIncludeDir) do
        externalincludedirs(v)
    end
end

usage "UE"
    generateIncludeDir_ue()
    filter "system:windows"
        libdirs
        {
            path.getabsolute("Lib/Win"),
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
            "UE-CoreUObject",
            "UE-InputCore",
            "UE-DesktopPlatform",
            "UE-DirectoryWatcher"
        }
        prebuildcommands {
            "{COPYFILE} %{wks.location}/External/UE/Lib/Win/*.dll %{cfg.targetdir}",
            "{COPYFILE} %{wks.location}/External/UE/Lib/Win/UE.modules %{cfg.targetdir}"
        }
        buildoptions {
            "/GR-", --UE modules disable rtti
        }
    filter "system:macosx"
        links 
        {
            "%{cfg.targetdir}/UE-Core.dylib",
            "%{cfg.targetdir}/UE-ApplicationCore.dylib",
            "%{cfg.targetdir}/UE-Slate.dylib",
            "%{cfg.targetdir}/UE-SlateCore.dylib",
            "%{cfg.targetdir}/UE-StandaloneRenderer.dylib",
            "%{cfg.targetdir}/UE-Projects.dylib",
            "%{cfg.targetdir}/UE-ImageWrapper.dylib",
            "%{cfg.targetdir}/UE-CoreUObject.dylib",
            "%{cfg.targetdir}/UE-InputCore.dylib",
            "%{cfg.targetdir}/UE-DesktopPlatform.dylib",
            "%{cfg.targetdir}/UE-DirectoryWatcher.dylib"
        }
        prebuildcommands {
            "{COPYFILE} %{wks.location}/External/UE/Lib/Mac/*.dylib %{cfg.targetdir}",
            "{COPYFILE} %{wks.location}/External/UE/Lib/Mac/UE.modules %{cfg.targetdir}"
        }
        buildoptions { 
            "-x objective-c++",
            "-fno-rtti", --UE modules disable rtti
        }

        

