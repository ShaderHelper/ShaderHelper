UnitTestFrameWorkHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp"},
}

project "UnitTestFrameWork"
    kind "WindowedApp"   
    location "%{_WORKING_DIR}/ProjectFiles"

    vpaths(UnitTestFrameWorkHierarchy)

    files {seq(UnitTestFrameWorkHierarchy)}

    includedirs
    {
        "./",
    }

    uses {
		"UE",
		"Framework",
	}

    filter "system:windows"
        pchheader "CommonHeader.h"
        pchsource "CommonHeader.cpp"

    filter "system:macosx"
        xcodebuildsettings { ["GENERATE_INFOPLIST_FILE"] = "YES" }



        

