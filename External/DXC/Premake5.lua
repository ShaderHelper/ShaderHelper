usage "DXC"
    filter "system:windows"
        prebuildcommands "{COPYFILE} %{wks.location}/External/DXC/bin/*.dll %{cfg.targetdir}"
        libdirs
        {
            path.getabsolute("./bin"),
        }
        includedirs
        {
            "./Inc",
        }

