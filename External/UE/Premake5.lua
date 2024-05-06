UeHierarchy = {
    ["External/UE/Src/**"] = {
		"%{_WORKING_DIR}/External/UE/Src/**.h",
		"%{_WORKING_DIR}/External/UE/Src/**.cpp", 
		"%{_WORKING_DIR}/External/UE/Src/**.hpp"
	},
	["External/UE/Shader/**"] = {
		"%{_WORKING_DIR}/External/UE/Shader/**.*",
	},
}

ue_module = {
    "Core", "TraceLog", "ApplicationCore", "Slate",
    "InputCore", "StandaloneRenderer", "CoreUObject",
    "SlateCore", "Json", "ImageWrapper", "DesktopPlatform",
    "DirectoryWatcher"
}

function generateIncludeDir_ue()
    local ueSrcDir = _WORKING_DIR .. "/External/UE/Src/"
    local ueUhtDir = _WORKING_DIR .. "/External/UE/Src/UHT/"
    externalincludedirs(ueSrcDir)
    externalwarnings("Off")

    for _ , module in pairs(ue_module) do
        externalincludedirs(ueSrcDir .. module .. "/Public")
        externalincludedirs(ueSrcDir .. module .. "/Internal")
        externalincludedirs(ueSrcDir .. module .. "/Classes")

        externalincludedirs(ueUhtDir .. module)
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
			"/experimental:external", --before vs2019 16.10
        }
    filter "system:macosx"
        links 
        {
            "%{cfg.targetdir}/UE.dylib",
        }
        prebuildcommands {
            "{COPYFILE} %{wks.location}/External/UE/Lib/*.dylib %{cfg.targetdir}",
            "{COPYFILE} %{wks.location}/External/UE/Lib/*.dSYM %{cfg.targetdir}",
        }
        postbuildcommands {
            "sh %{wks.location}/External/UE/lldbinit.sh",
            "python3 %{wks.location}/External/dsymPlist.py --dsymFilePath %{cfg.targetdir}/UE.dSYM --srcDir %{wks.location}/External/UE/Src --buildSrcDir /UePlaceholder --relativePath"
        }
        buildoptions { 
            "-x objective-c++",
            "-fno-rtti", --UE modules disable rtti
        }

        

