fbxHierarchy = {
    ["External/Fbx/*"] = {"%{_WORKING_DIR}/External/Fbx/**"},
}

usage "fbx"
    externalincludedirs
    {
        "./Include",
    }
    libdirs
    {
        path.getabsolute("./Lib"),
    }
    defines 
    {
        "FBXSDK_SHARED",
    }
    filter "system:macosx"
        links
        {
            "fbxsdk"
        }
        prebuildcommands "{COPYFILE} %{wks.location}/External/Fbx/Lib/*.dylib %{cfg.targetdir}"

    filter "system:windows"
        links
        {
            "libfbxsdk"
        }
        prebuildcommands "{COPYFILE} \"%{wks.location}/External/Fbx/Lib/*.dll\" %{cfg.targetdir}"
