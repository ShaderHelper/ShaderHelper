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

require('vstudio')
p.api.register {
  name = "workspace_items",
  scope = "workspace",
  kind = "list:keyed:list:string",
}

workspaceFolderUUID = {}

function addWorkspaceItemsByFolder(wks, folders, parentFolderUUID)
    for name, subPaths in pairs(folders) do
        local folderUUID = os.uuid("Solution Items:"..wks.name .. ":" .. path.getdirectory(subPaths[1]))

        if parentFolderUUID then
            workspaceFolderUUID[folderUUID] = parentFolderUUID
        end

        local fileSections = {}
        local nextFolders = {}
        for _, pathValue in ipairs(subPaths) do
            if os.isdir(pathValue) then
                local nextFolderSubPaths = {}
                local folderName = path.getbasename(pathValue)
                for _, subFilePath in pairs(os.matchfiles(pathValue .. "/*")) do
                    table.insert(nextFolderSubPaths, subFilePath)
                end
                for _, subDirPath in pairs(os.matchdirs(pathValue .. "/*")) do
                    table.insert(nextFolderSubPaths, subDirPath)
                end
                nextFolders[folderName] = nextFolderSubPaths                
            else
                for _, file in ipairs(os.matchfiles(pathValue)) do
                    file = path.rebase(file, ".", wks.location)
                    table.insert(fileSections, file)
                end
            end
        end

        p.push('Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "'..name..'", "'..name..'", "{' .. folderUUID .. '}"')
        p.push("ProjectSection(SolutionItems) = preProject")

        for _, fileSection in ipairs(fileSections) do
            p.w(fileSection.." = "..fileSection)
        end

        p.pop("EndProjectSection")
        p.pop("EndProject")

        addWorkspaceItemsByFolder(wks, nextFolders, folderUUID)
    end
end

p.override(p.vstudio.sln2005, "projects", function(base, wks)
    for _, items in ipairs(wks.workspace_items) do
        addWorkspaceItemsByFolder(wks, items, nil)
    end
  base(wks)
end)

p.override(p.vstudio.sln2005, "nestedProjects", function(base, wks)
	p.push("GlobalSection(NestedProjects) = preSolution")
    local tr = p.workspace.grouptree(wks)
	if p.tree.hasbranches(tr) then
		p.tree.traverse(tr, {
			onnode = function(n)
				if n.parent.uuid then
					p.w("{%s} = {%s}", (n.project or n).uuid, n.parent.uuid)
				end
			end
		})
	end

    for UUID, parentUUID in pairs(workspaceFolderUUID) do
        p.w("{%s} = {%s}", UUID, parentUUID)
    end

	p.pop("EndGlobalSection")
end)

-- Overrides project bake
p.override(p.project, "bake", function(base, self)
    bakeProjectUsage( self ) 

    return base(self)
end)
