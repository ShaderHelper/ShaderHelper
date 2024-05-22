DXCHierarchy = {
    ["External/DXC"] = {"%{_WORKING_DIR}/External/DXC/Inc/*.h"},
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

