#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading UE..."
    UE_Lib_Dir="$CurrentPath/Lib"
    UE_Lib_Url0="https://gitlab.com/-/project/55718299/uploads/ee1b49cc24eec8151ad3d37dfb045f6d/UE-Lib-Mac.zip.00"
    UE_Lib_Url1="https://gitlab.com/-/project/55718299/uploads/ed3a254e0c91ef07b95ac6f659013c01/UE-Lib-Mac.zip.01"
    cd "$UE_Lib_Dir" || exit 1
    curl -LO $UE_Lib_Url0 || exit 1
    curl -LO $UE_Lib_Url1 || exit 1
    cat UE-Lib-Mac.zip.* > UE-Lib-Mac.zip || exit 1
    tar -zxf UE-Lib-Mac.zip || exit 1
    rm UE-Lib-Mac.zip*

    UE_Src_Dir="$CurrentPath/Src"
    UE_Src_Url="https://gitlab.com/-/project/55718299/uploads/144c0bc8e5d41f321f9fcf0d1b75ac68/UE-Src.tar.gz"
    cd "$UE_Src_Dir" || exit 1
    curl -LO $UE_Src_Url || exit 1
    tar -zxf UE-Src.tar.gz || exit 1
    rm UE-Src.tar.gz
echo

exit 0
