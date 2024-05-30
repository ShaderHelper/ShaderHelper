metalcppHierarchy = {
    ["External/metal-cpp/*"] = {"%{_WORKING_DIR}/External/metal-cpp/**"},
}

usage "metal-cpp"
    includedirs
    {
        path.getabsolute("./"),
    }

