usage "DXC"
    filter "system:windows"
		postbuildcommands '{COPY} "%{wks.location}/External/DXC/bin/*.dll" "%{cfg.targetdir}"'

