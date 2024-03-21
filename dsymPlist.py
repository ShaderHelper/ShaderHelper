
import plistlib
import argparse
import os
from os import path
import subprocess
import shutil

bundle_resource_path = "Contents/Resources"
bundle_dwarf_path = path.join(bundle_resource_path, "DWARF")

def create_dsym_bundle(dsymFilePath):
    dsym_bundle_path = path.dirname(dsymFilePath) + "/TempDsymBundle"
    os.makedirs(path.join(dsym_bundle_path, bundle_dwarf_path))
    return dsym_bundle_path

def parse_dsym(dsymFilePath):
    uuid_info = subprocess.run("dwarfdump --uuid \"{}\"".format(dsymFilePath), shell=True, capture_output=True, text=True).stdout
    lines = uuid_info.strip().split("\n")
    arch_to_uuid = {}
    for line in lines:
        elements = line.split(" ")
        arch_to_uuid[elements[2].replace("(", "").replace(")", "")] = elements[1]
    return arch_to_uuid

def fill_bundle(dsym_bundle_path, arch_to_uuid, dsymFilePath, srcDir, buildSrcDir):
    shutil.move(dsymFilePath, path.join(dsym_bundle_path, bundle_dwarf_path))
    #gen plist
    for arch, uuid in arch_to_uuid.items():
        plist_dict = {
            "DBGArchitecture" : arch,
            "DBGBuildSourcePath" : buildSrcDir,
            "DBGSourcePath" : srcDir
        }
        plist_path = path.join(dsym_bundle_path, bundle_resource_path, uuid + ".plist")
        plist_file_object = open(plist_path, "wb")
        plistlib.dump(plist_dict, plist_file_object)

def main():
    parser = argparse.ArgumentParser(description="Generate the plist for a dSYM")
    parser.add_argument("--dsymFilePath", required=True)
    parser.add_argument("--srcDir", required=True)
    parser.add_argument("--buildSrcDir", required=True)

    args = parser.parse_args()
    dsym_bundle_path = create_dsym_bundle(args.dsymFilePath)

    arch_to_uuid = parse_dsym(args.dsymFilePath)
    fill_bundle(dsym_bundle_path, arch_to_uuid, args.dsymFilePath, args.srcDir, args.buildSrcDir)

    origin_dsym_name = path.splitext(path.basename(args.dsymFilePath))[0]
    new_dsym_name = origin_dsym_name + ".dylib.dSYM"
    os.rename(dsym_bundle_path, path.join(path.dirname(dsym_bundle_path), new_dsym_name))

main()