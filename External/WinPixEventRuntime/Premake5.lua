usage "WinPixEventRuntime"
    filter "system:windows"
	    postbuildcommands '{COPY} "%{wks.location}/External/WinPixEventRuntime/*.dll" "%{cfg.targetdir}"'
        libdirs
        {
            path.getabsolute("./"),
        }
        includedirs
        {
            "./Inc",
        }
        links "WinPixEventRuntime"

