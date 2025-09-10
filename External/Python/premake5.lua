pythonHierarchy = {
    ["External/Python/*"] = {
        "%{_WORKING_DIR}/External/Python/Include/**", 
        "%{_WORKING_DIR}/External/Python/pybind11/**",
    },
}

usage "Python"
    externalincludedirs
    {
        "./Include",
        "./"
    }
    libdirs
    {
        path.getabsolute("./"),
    }
    filter "system:windows"
        prebuildcommands {
            "{COPYFILE} \"%{wks.location}/External/Python/python311.dll\" %{cfg.targetdir}",
            "{COPY} \"%{wks.location}/External/Python/DLLs\" %{cfg.targetdir}/Python/DLLs",
            "{COPY} \"%{wks.location}/External/Python/Lib\" %{cfg.targetdir}/Python/Lib",
        }
        links
        {
            "python311",
        }
    filter "system:macosx"
        prebuildcommands {
            "mkdir -p %{cfg.targetdir}/Python/lib",
            "{COPYFILE} %{wks.location}/External/Python/lib/libpython3.11.dylib %{cfg.targetdir}",
            "{COPY} %{wks.location}/External/Python/lib/python3.11 %{cfg.targetdir}/Python/lib",
        }
        links
        {
            "python3.11",
        }