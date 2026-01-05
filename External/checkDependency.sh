#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

Check() {
    if [ "$(uname)" = "Darwin" ]; then
        ModulePath="$CurrentPath/$1/downloadDep.sh"
    elif [ "$(uname)" = "Linux" ]; then
        ModulePath="$CurrentPath/$1/downloadDepLinux.sh"
    else
        ModulePath="$CurrentPath/$1/downloadDep.bat"
    fi

    DiffOutput=$(git diff HEAD@{1} --stat -- "$ModulePath")

    if [ ! -z "$DiffOutput" ]; then
        echo "$1 has changed, trying to download the latest version!!"
        if [ "$(uname)" = "Darwin" ] || [ "$(uname)" = "Linux" ]; then
           sh "$ModulePath"
        else
           "$ModulePath"
        fi
    fi

}

echo "Checking dependencies..."
if [ "$(uname)" = "Darwin" ]; then
    Check "ShaderConductor"
    Check "UE"
    Check "Python"
elif [ "$(uname)" = "Linux" ]; then
    Check "UE"
else
    Check "AgilitySDK"
    Check "ShaderConductor"
    Check "UE"
    Check "Python"
	Check "shaderc"
fi
