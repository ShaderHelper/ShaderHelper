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

function addToProjectHierarchy(Hierarchy)
	vpaths(Hierarchy)
    files {seq(Hierarchy)}
end

require('vstudio')
p.api.register {
  name = "workspace_items",
  scope = "workspace",
  kind = "table",
}

workspaceFolderUUID = {}

function addWorkspaceItemsByFolder(wks, folders, parentFolderUUID)
    for name, subPaths in pairs(folders) do
        local fileSections = {}
        local nextFolders = {}
        local resolvePath = function(pathValue)
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

        local folderUUID
        if type(subPaths) == "string" then
            folderUUID = os.uuid("Solution Items:"..wks.name .. ":" .. path.getdirectory(subPaths))
            resolvePath(subPaths)
        else
            folderUUID = os.uuid("Solution Items:"..wks.name .. ":" .. path.getdirectory(subPaths[1]))
            for _, pathValue in ipairs(subPaths) do
                resolvePath(pathValue)
            end
        end

        if parentFolderUUID then
            workspaceFolderUUID[folderUUID] = parentFolderUUID
        end

        if type(name) == "number" then
            addWorkspaceItemsByFolder(wks, nextFolders, nil)
        else
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
end

p.override(p.vstudio.sln2005, "projects", function(base, wks)
  addWorkspaceItemsByFolder(wks, wks.workspace_items, nil)
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
