

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
            "UE-InputCore"
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
            "%{cfg.targetdir}/UE-InputCore.dylib"
        }
        prebuildcommands {
            "{COPYFILE} %{wks.location}/External/UE/Lib/Mac/*.dylib %{cfg.targetdir}",
            "{COPYFILE} %{wks.location}/External/UE/Lib/Mac/UE.modules %{cfg.targetdir}"
        }
        buildoptions { 
            "-x objective-c++",
            "-fno-rtti", --UE modules disable rtti
        }

        

