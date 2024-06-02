magic_enumHierarchy = {
    ["External/magic_enum/*"] = {"%{_WORKING_DIR}/External/magic_enum/**"},
}

usage "magic_enum"
    includedirs
    {
        path.getabsolute("./"),
    }

