MtlppUEHierarchy = {
    ["External/MtlppUE"] = {
        "%{_WORKING_DIR}/External/MtlppUE/src/*.hpp", 
        "%{_WORKING_DIR}/External/MtlppUE/src/*.mm", 
        "%{_WORKING_DIR}/External/MtlppUE/src/*.inl"
    },
}

usage "MtlppUE"
    filter "system:macosx"
        includedirs
        {
            path.getabsolute("./src"),
        }

        vpaths(MtlppUEHierarchy)
        files {seq(MtlppUEHierarchy)}

    filter {"files:**/command_encoder.mm"}
        flags {"ExcludeFromBuild"}
    
