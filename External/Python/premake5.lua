pythonHierarchy = {
    ["External/Python/*"] = {
        "%{_WORKING_DIR}/External/Python/Include/**", 
        "%{_WORKING_DIR}/External/Python/pybind11/**",
    },
}

usage "Python"
    includedirs
    {
        "./Include",
        "./"
    }
    libdirs
    {
        path.getabsolute("./"),
    }
    links
    {
        "python311",
    }
    filter "system:windows"
        prebuildcommands {
            "{COPYFILE} \"%{wks.location}/External/Python/python311.dll\" %{cfg.targetdir}",
            "{COPY} \"%{wks.location}/External/Python/DLLs\" %{cfg.targetdir}/Python/DLLs",
            "{COPY} \"%{wks.location}/External/Python/Lib\" %{cfg.targetdir}/Python/Lib",
        } 
