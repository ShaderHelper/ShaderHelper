include "usage.lua"

local p = premake

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
