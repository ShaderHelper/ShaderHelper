usage "AgilitySDK"
    filter "system:windows"
		postbuildcommands '{COPY} "%{wks.location}/External/AgilitySDK/*.dll" "%{cfg.targetdir}/AgilitySDK"'

