#!/bin/bash
#
# Prepare ZIP files for upload as release files on Github.
#
# Before running this script,
# you need to compile the binaries for all platforms
# and store them in a local `release/` folder:
#
# $ tree release
# release/
# ├── include
# │   └── smileapi
# │       └── SMILEapi.h
# ├── licenses
# │   ├── LibSVM.txt
# │   ├── LICENSE
# │   ├── newmat.txt
# │   ├── Rapidjson.txt
# │   └── Speex.txt
# ├── linux-armv7
# │   ├── bin
# │   │   └── SMILExtract
# │   └── lib
# │       └── libSMILEapi.so
# ├── linux-armv8
# │   ├── bin
# │   │   └── SMILExtract
# │   └── lib
# │       └── libSMILEapi.so
# ├── linux-x86_64
# │   ├── bin
# │   │   └── SMILExtract
# │   └── lib
# │       └── libSMILEapi.so
# ├── macos-armv8
# │   ├── bin
# │   │   └── SMILExtract
# │   └── lib
# │       └── libSMILEapi.dylib
# ├── macos-x86_64
# │   ├── bin
# │   │   └── SMILExtract
# │   └── lib
# │       └── libSMILEapi.dylib
# └── windows-x86_64
#     ├── bin
#     │   └── SMILExtract.exe
#     └── lib
#         ├── SMILEapi.dll
#         └── SMILEapi.lib
# 
# 21 directories, 19 files
#
# Those binaries can be build automatically
# using our internal CI pipeline at
# https://gitlab.audeering.com/tools/opensmile-ci.
# Just make sure to first increase the version inside `conanfile.py`
# and commit those changes as release,
# and run the CI pipeline on that commit.

# === Get version number ===
# Extract version from conanfile,
# see https://stackoverflow.com/a/43644495
version=$(grep -Po '\bversion\s*=\s*"\K.*?(?=")' conanfile.py)

# === Prepare docs ===
# Create virtual environment,
# build documentaton
# and store inside the downloaded release folder.
rm -rf venv
virtualenv -p python3.10 venv
source venv/bin/activate
pip install -r doc/sphinx/requirements.txt.lock
python -m sphinx "doc/sphinx/" "release/doc" -b html
deactivate
rm -rf venv


# === Copy file to specific release folders ===
for architecture in "linux-armv7" "linux-armv8" "linux-x86_64" "macos-armv8" "macos-x86_64" "windows-x86_64"; do

    release="opensmile-$version-$architecture"

    rm -rf "$release"
    mkdir "$release"
    cp -R "release/$architecture/bin" "$release/bin"
    cp -R release/$architecture/lib/* "$release/bin/"

    # config
    cp -R "config" "$release/config"

    # doc
    cp -R "release/doc" "$release/doc"

    # example-audio
    cp -R "example-audio" "$release/example-audio"

    # include
    cp -R "release/include" "$release/include"

    # licenses
    cp -R "release/licenses" "$release/licenses"

    # scripts
    cp -R "scripts" "$release/scripts"

    # CHANGELOG.md
    cp "CHANGELOG.md" "$release/CHANGELOG.md"

    # LICENSE
    cp "LICENSE" "$release/LICENSE"

    # README.md
    cp "README.md" "$release/README.md"

    # Create ZIP archive
    zip "$release.zip" "$release"
done
