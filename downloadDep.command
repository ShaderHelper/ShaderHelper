#!/bin/sh
echo
CurrentPath="$(cd "$(dirname "$0")" && pwd)"
cd "$CurrentPath"

function Error() {
    echo "Erros encountered during execution."
    echo
    exit 1
}

echo #!/bin/sh > .git/hooks/post-merge
echo ./External/CheckDependency.sh >> .git/hooks/post-merge

sh "$CurrentPath/External/ShaderConductor/downloadDep.sh" || Error
sh "$CurrentPath/External/UE/downloadDep.sh" || Error

echo "Script Complete"
exit 0
