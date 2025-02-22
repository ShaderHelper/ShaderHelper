shaderConductorHierarchy = {
    ["External/ShaderConductor/*"] = {"%{_WORKING_DIR}/External/ShaderConductor/**"},
}

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
            "dxcompiler"
        }
        includedirs
        {
            "./Include",
        }
        buildoptions {
            "-fms-extensions", --dxc uuid requires it for clang
        }

