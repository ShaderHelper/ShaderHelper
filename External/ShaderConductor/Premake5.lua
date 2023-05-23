usage "ShaderConductor"
    filter "system:macosx"
        prebuildcommands "{COPYFILE} %{wks.location}/External/ShaderConductor/Lib/Mac/*.dylib %{cfg.targetdir}"
        libdirs
        {
            path.getabsolute("./Lib/Mac"),
        }
        links
        {
            "ShaderConductor",
        }
        includedirs
        {
            "./Include",
        }

