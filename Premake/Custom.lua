include "usage.lua"

local p = premake

UE_BaseIncludeDir = {
    Core = _WORKING_DIR .. "/External/UE/Include/Core",
    TraceLog = _WORKING_DIR .. "/External/UE/Include/TraceLog",
    ApplicationCore = _WORKING_DIR .. "/External/UE/Include/ApplicationCore",
    Slate = _WORKING_DIR .. "/External/UE/Include/Slate",
    InputCore = _WORKING_DIR .. "/External/UE/Include/InputCore",
    StandaloneRenderer = _WORKING_DIR .. "/External/UE/Include/StandaloneRenderer",
    CoreUObject = _WORKING_DIR .. "/External/UE/Include/CoreUObject",
    SlateCore = _WORKING_DIR .. "/External/UE/Include/SlateCore",
    Json = _WORKING_DIR .. "/External/UE/Include/Json",
    ImageWrapper = _WORKING_DIR .. "/External/UE/Include/ImageWrapper",
}

UE_UHTIncludeDir = {
    InputCore = _WORKING_DIR .. "/External/UE/Include/InputCore/UHT",
    Slate = _WORKING_DIR .. "/External/UE/Include/Slate/UHT",
    CoreUObject = _WORKING_DIR .. "/External/UE/Include/CoreUObject/UHT",
    SlateCore = _WORKING_DIR .. "/External/UE/Include/SlateCore/UHT",
}

function GenerateIncludeDir_UE()
    externalincludedirs(_WORKING_DIR .. "/External/UE/Include")

    for _, v in pairs(UE_BaseIncludeDir) do
        externalincludedirs(v)
    end

    for _, v in pairs(UE_UHTIncludeDir) do
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
