#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading UE..."
    UE_Lib_Dir="$CurrentPath/Lib"
    UE_Lib_Url0="https://gitlab.com/-/project/55718299/uploads/2e3026c2829e5945246c02392dea49d3/UE-Lib-Mac.zip.00"
    UE_Lib_Url1="https://gitlab.com/-/project/55718299/uploads/35d36b61cb3aa5dd32c57de0cd8e223e/UE-Lib-Mac.zip.01"
    cd "$UE_Lib_Dir" || exit 1
    curl -LO $UE_Lib_Url0 || exit 1
    curl -LO $UE_Lib_Url1 || exit 1
    cat UE-Lib-Mac.zip.* > UE-Lib-Mac.zip || exit 1
    tar -zxf UE-Lib-Mac.zip || exit 1
    rm UE-Lib-Mac.zip*

    UE_Src_Dir="$CurrentPath/Src"
    UE_Src_Url="https://gitlab.com/-/project/55718299/uploads/31b1f551b1050d9ce6dfa2a6bffb16d2/UE-Src.tar"
    cd "$UE_Src_Dir" || exit 1
    curl -LO $UE_Src_Url || exit 1
    tar -zxf UE-Src.tar || exit 1
    rm UE-Src.tar
echo

exit 0
