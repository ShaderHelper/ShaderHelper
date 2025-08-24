shaderConductorHierarchy = {
    ["External/ShaderConductor/*"] = {"%{_WORKING_DIR}/External/ShaderConductor/**"},
}

usage "ShaderConductor"
    includedirs
    {
        "./Include",
    }
    libdirs
    {
        path.getabsolute("./Lib"),
    }
    links
    {
        "ShaderConductor",
        "dxcompiler"
    }
    filter "system:macosx"
        prebuildcommands "{COPYFILE} %{wks.location}/External/ShaderConductor/Lib/*.dylib %{cfg.targetdir}"
        buildoptions {
            "-fms-extensions", --dxc uuid requires it for clang
        }
    filter "system:windows"
        prebuildcommands "{COPYFILE} \"%{wks.location}/External/ShaderConductor/Lib/*.dll\" %{cfg.targetdir}"
