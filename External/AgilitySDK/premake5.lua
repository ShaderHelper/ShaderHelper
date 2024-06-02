AgilitySDKHierarchy = {
    ["External/AgilitySDK/*"] = {"%{_WORKING_DIR}/External/AgilitySDK/**"},
}

usage "AgilitySDK"
    filter "system:windows"
        --xcopy /Q /Y /I
        prebuildcommands "{COPY} \"%{wks.location}/External/AgilitySDK/Lib/*.dll\" %{cfg.targetdir}/AgilitySDK"
        includedirs
        {
            "./Inc",
        }
