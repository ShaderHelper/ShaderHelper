usage "WinPixEventRuntime"
    filter "system:windows"
        prebuildcommands "{COPYFILE} %{wks.location}/External/WinPixEventRuntime/*.dll %{cfg.targetdir}"
        libdirs
        {
            path.getabsolute("./"),
        }
        includedirs
        {
            "./Inc",
        }
        links "WinPixEventRuntime"

