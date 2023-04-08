
usage "MtlppUE"
    includedirs
    {
        path.getabsolute("./src"),
    }
    filter {"files:**/command_encoder.mm"}
        flags {"ExcludeFromBuild"}
    
