magic_enumHierarchy = {
    ["External/magic_enum"] = {"%{_WORKING_DIR}/External/magic_enum/magic_enum.hpp"},
}

usage "magic_enum"
    includedirs
    {
        path.getabsolute("./"),
    }

    vpaths(magic_enumHierarchy)
    files {seq(magic_enumHierarchy)}

