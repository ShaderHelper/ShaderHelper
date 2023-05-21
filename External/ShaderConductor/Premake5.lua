usage "ShaderConductor"
    filter "system:macos"
        prebuildcommands "{COPYFILE} %{wks.location}/External/ShaderConductor/Lib/Mac/*.dylib %{cfg.targetdir}"
        libdirs
        {
            path.getabsolute("./Lib/Mac"),
        }
        includedirs
        {
            "./Include",
        }

