
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
            path.getabsolute("Lib"),
        }
        links
        {
            "UE",
        }
        prebuildcommands {
            "{COPYFILE} \"%{wks.location}/External/UE/Lib/*.dll\" %{cfg.targetdir}",
			"{COPYFILE} \"%{wks.location}/External/UE/Lib/*.pdb\" %{cfg.targetdir}"
        }
        buildoptions {
            "/GR-", --UE modules disable rtti
        }
    filter "system:macosx"
        links 
        {
            "%{cfg.targetdir}/UE.dylib",
        }
        prebuildcommands {
            "{COPYFILE} %{wks.location}/External/UE/Lib/*.dylib %{cfg.targetdir}",
            "{COPYFILE} -r %{wks.location}/External/UE/Lib/*.dSYM %{cfg.targetdir}"
        }
        buildoptions { 
            "-x objective-c++",
            "-fno-rtti", --UE modules disable rtti
        }

        

