#!/bin/sh
echo
CurrentPath="$(cd "$(dirname "$0")" && pwd)"
cd "$CurrentPath"

Error() {
    echo "Erros encountered during execution."
    echo
    exit 1
}

echo "#!/bin/sh" > .git/hooks/post-merge
echo "sh External/checkDependency.sh" >> .git/hooks/post-merge
chmod ug+x .git/hooks/*

OS_TYPE=$(uname)
case "$OS_TYPE" in
    Linux)
        sh "$CurrentPath/External/UE/downloadDepLinux.sh" || Error
        ;;
    Darwin)
        sh "$CurrentPath/External/ShaderConductor/downloadDep.sh" || Error
        sh "$CurrentPath/External/UE/downloadDep.sh" || Error
        ;;
    *)
        Error
esac

echo "Script Complete"
exit 0