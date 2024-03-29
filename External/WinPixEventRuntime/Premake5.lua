WinPixEventRuntimeHierarchy = {
    ["External/WinPixEventRuntime"] = {"%{_WORKING_DIR}/External/WinPixEventRuntime/Inc/*.h"},
}

usage "WinPixEventRuntime"
    filter "system:windows"
        prebuildcommands "{COPYFILE} \"%{wks.location}/External/WinPixEventRuntime/Lib/*.dll\" %{cfg.targetdir}"
        libdirs
        {
            path.getabsolute("./Lib"),
        }
        includedirs
        {
            "./Inc",
        }
        links "WinPixEventRuntime"

        vpaths(WinPixEventRuntimeHierarchy)
        files {seq(WinPixEventRuntimeHierarchy)}

