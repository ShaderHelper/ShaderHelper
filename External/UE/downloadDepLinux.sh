#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading UE..."
    UE_Lib_Dir="$CurrentPath/Lib"
    UE_Lib_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/472ff39cf6d8eca1c6e594cc248e92e3/UE-Lib-Linux.tar"
    cd "$UE_Lib_Dir" || exit 1
    wget $UE_Lib_Url || exit 1
    tar -zxf UE-Lib-Linux.tar || exit 1
    rm UE-Lib-Linux.tar

    UE_Src_Dir="$CurrentPath/Src"
    UE_Src_Url="https://gitlab.com/-/project/55718299/uploads/31b1f551b1050d9ce6dfa2a6bffb16d2/UE-Src.tar"
    cd "$UE_Src_Dir" || exit 1
    wget $UE_Src_Url || exit 1
    tar -zxf UE-Src.tar || exit 1
    rm UE-Src.tar
echo

exit 0
