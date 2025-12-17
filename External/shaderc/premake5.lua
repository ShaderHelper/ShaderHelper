shadercHierarchy = {
    ["External/shaderc/*"] = {"%{_WORKING_DIR}/External/shaderc/**"},
}

usage "shaderc"
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
        "shaderc_shared",
    }
    filter "system:macosx"
        prebuildcommands "{COPYFILE} %{wks.location}/External/shaderc/Lib/*.dylib %{cfg.targetdir}"

    filter "system:windows"
        prebuildcommands "{COPYFILE} \"%{wks.location}/External/shaderc/Lib/*.dll\" %{cfg.targetdir}"
