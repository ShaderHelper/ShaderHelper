DXCHierarchy = {
    ["External/DXC/*"] = {"%{_WORKING_DIR}/External/DXC/**"},
}

usage "DXC"
    filter "system:windows"
        prebuildcommands "{COPYFILE} \"%{wks.location}/External/DXC/Lib/*.dll\" %{cfg.targetdir}"
        libdirs
        {
            path.getabsolute("./Lib"),
        }
        includedirs
        {
            "./Inc",
        }

