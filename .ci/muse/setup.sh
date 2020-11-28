#!/usr/bin/env bash

# Reference: https://docs.muse.dev/docs/configuring-muse/

apt-get -u update
./get-dependencies_linux.sh ubuntu build-all

# Initialize Git submodules
git submodule update --init --recursive

# Create the build directory
mkdir build

# # MuseDev seems to currently run in-source builds
# # Truncate the DisallowInSourceBuilds.cmake file to allow this (for now)
# echo "" > ./cmake/DisallowInSourceBuilds.cmake
