include "usage.lua"

local p = premake

ue_baseIncludeDir = {
    core = _WORKING_DIR .. "/External/UE/Include/Core",
    traceLog = _WORKING_DIR .. "/External/UE/Include/TraceLog",
    applicationCore = _WORKING_DIR .. "/External/UE/Include/ApplicationCore",
    slate = _WORKING_DIR .. "/External/UE/Include/Slate",
    inputCore = _WORKING_DIR .. "/External/UE/Include/InputCore",
    standaloneRenderer = _WORKING_DIR .. "/External/UE/Include/StandaloneRenderer",
    coreUObject = _WORKING_DIR .. "/External/UE/Include/CoreUObject",
    slateCore = _WORKING_DIR .. "/External/UE/Include/SlateCore",
    json = _WORKING_DIR .. "/External/UE/Include/Json",
    imageWrapper = _WORKING_DIR .. "/External/UE/Include/ImageWrapper",
}

ue_uhtIncludeDir = {
    inputCore = _WORKING_DIR .. "/External/UE/Include/InputCore/UHT",
    slate = _WORKING_DIR .. "/External/UE/Include/Slate/UHT",
    coreUObject = _WORKING_DIR .. "/External/UE/Include/CoreUObject/UHT",
    slateCore = _WORKING_DIR .. "/External/UE/Include/SlateCore/UHT",
}

function generateIncludeDir_ue()
    externalincludedirs(_WORKING_DIR .. "/External/UE/Include")

    for _, v in pairs(ue_baseIncludeDir) do
        externalincludedirs(v)
    end

    for _, v in pairs(ue_uhtIncludeDir) do
        externalincludedirs(v)
    end
end

function seq(tab)
    local seq = {}
    for key,value in pairs(tab) do
        for index,element in pairs(value) do
            table.insert(seq, element)
        end
    end
    return seq
end

-- Overrides project bake
premake.override(p.project, "bake", function(base, self)
    bakeProjectUsage( self ) 

    return base(self)
end)
