#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading Python..."
    Python_Dir="$CurrentPath"
    Python_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/Python-Mac.zip"
    cd "$Python_Dir" || exit 1
    curl -LO $Python_Url || exit 1
    tar -zxf Python-Mac.zip || exit 1
    rm Python-Mac.zip
echo
 
exit 0
