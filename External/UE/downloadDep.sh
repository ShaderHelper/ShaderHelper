#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading UE..."
    UE_Lib_Dir="$CurrentPath/Lib"
    UE_Lib_Url0="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/UE-Lib-Mac.zip.00"
    UE_Lib_Url1="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/UE-Lib-Mac.zip.01"
    cd "$UE_Lib_Dir" || exit 1
    curl -LO $UE_Lib_Url0 || exit 1
    curl -LO $UE_Lib_Url1 || exit 1
    cat UE-Lib-Mac.zip.* > UE-Lib-Mac.zip || exit 1
    tar -zxf UE-Lib-Mac.zip || exit 1
    rm UE-Lib-Mac.zip*

    UE_Src_Dir="$CurrentPath/Src"
    UE_Src_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/UE-Src.tar.gz"
    cd "$UE_Src_Dir" || exit 1
    curl -LO $UE_Src_Url || exit 1
    tar -zxf UE-Src.tar.gz || exit 1
    rm UE-Src.tar.gz
echo

exit 0
