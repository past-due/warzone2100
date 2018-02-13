#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script cleans up warzone.

##############################
# General Setup

. macosx/BuildBot/_xcodebuild_helpers.sh


##############################
# Clean Warzone

cd macosx

if ! execute_xcodebuild_command clean -project Warzone.xcodeproj -target "Warzone" -configuration "Release" -destination "platform=macOS"; then
	exit ${?}
fi

rm -rf build/Release

exit ${?}
