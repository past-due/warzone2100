#!/usr/bin/env bash

# Reference: https://docs.muse.dev/docs/configuring-muse/

apt-get -u update
./get-dependencies_linux.sh ubuntu build-all

# Initialize Git submodules
git submodule update --init --recursive

# Create the build directory
mkdir build

# Run CMake
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B ./build .

# Copy compile commands database to the location that MuseDev expects it
mv build/compile_commands.json ./
