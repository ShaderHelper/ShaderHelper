#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

function Check() {

    if [[ "$OSTYPE" == "darwin"* ]]; then
        ModulePath="$CurrentPath/$1/downloadDep.sh"
    else
        ModulePath="$CurrentPath/$1/downloadDep.bat"
    fi

    DiffOutput=$(git diff HEAD@{1} --stat -- "$ModulePath")

    if [ ! -z "$DiffOutput" ]; then
        echo "$1 has changed, trying to download the latest version!!"
        if [[ "$OSTYPE" == "darwin"* ]]; then
           sh "$ModulePath"
        else
           "$ModulePath"
        fi
    fi

}

echo "Checking dependencies..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    Check "ShaderConductor"
    Check "UE"
else
    Check "AgilitySDK"
    Check "DXC"
    Check "UE"
fi

