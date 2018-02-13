#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script cleans up warzone.

##############################
# General Setup

. macosx/BuildBot/_xcodebuild_detect.sh


##############################
# Clean Warzone

cd macosx

$XCODEBUILD \
 -project Warzone.xcodeproj \
 -target "Warzone" \
 -configuration "Release" \
 -destination "platform=macOS" \
 clean \
 $XCPRETTY

exit ${?}
