fbxHierarchy = {
    ["External/Fbx/*"] = {"%{_WORKING_DIR}/External/Fbx/**"},
}

usage "fbx"
    includedirs
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
    links
    {
        "libfbxsdk"
    }
    filter "system:macosx"
        prebuildcommands "{COPYFILE} %{wks.location}/External/Fbx/Lib/*.dylib %{cfg.targetdir}"

    filter "system:windows"
        prebuildcommands "{COPYFILE} \"%{wks.location}/External/Fbx/Lib/*.dll\" %{cfg.targetdir}"
