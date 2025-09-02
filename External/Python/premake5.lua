pythonHierarchy = {
    ["External/Python/*"] = {"%{_WORKING_DIR}/External/Python/**"},
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
            "{COPY} \"%{wks.location}/External/Python/DLLs\" %{cfg.targetdir}/Python",
            "{COPYFILE} \"%{wks.location}/External/Python/python311.zip\" %{cfg.targetdir}/Python",
        } 
