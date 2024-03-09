usage "ShaderConductor"
    filter "system:macosx"
        prebuildcommands "{COPYFILE} %{wks.location}/External/ShaderConductor/Lib/*.dylib %{cfg.targetdir}"
        libdirs
        {
            path.getabsolute("./Lib"),
        }
        links
        {
            "ShaderConductor",
        }
        includedirs
        {
            "./Include",
        }

