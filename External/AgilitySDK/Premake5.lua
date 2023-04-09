usage "AgilitySDK"
    filter "system:windows"
        --xcopy /Q /Y /I
        prebuildcommands "{COPY} %{wks.location}/External/AgilitySDK/*.dll %{cfg.targetdir}/AgilitySDK"
        includedirs
        {
            "./Inc",
        }

