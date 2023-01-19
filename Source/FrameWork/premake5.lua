FrameWorkHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp"},
}

project "FrameWork"
    kind "SharedLib"   
    location "%{_WORKING_DIR}/ProjectFiles"

    vpaths(FrameWorkHierarchy)

    files {seq(FrameWorkHierarchy)}

    includedirs
    {
        "./",
    }

	defines {
		"FRAMEWORK_API=DLLEXPORT"
	}

    uses "UE"

    filter "system:windows"
        pchheader "CommonHeader.h"
        pchsource "CommonHeader.cpp"

usage "FrameWork"
	defines {
		"FRAMEWORK_API=DLLIMPORT"
	}
	includedirs
    {
        path.getabsolute("../"),
    }



        

