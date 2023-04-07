usage "AgilitySDK"
    filter "system:windows"
		prebuildcommands "{COPYFILE} %{wks.location}/External/AgilitySDK/*.dll %{cfg.targetdir}/AgilitySDK"

