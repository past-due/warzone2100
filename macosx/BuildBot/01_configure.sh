#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This just tries to download the external libs.  It does it twice because they may not download properly the first time.

##############################
# General Setup

. macosx/BuildBot/_xcodebuild_detect.sh


##############################
# Download External Libs

cd macosx

if ! $XCODEBUILD -project Warzone.xcodeproj -target "Fetch Third Party Sources" $XCPRETTY; then
	if ! $XCODEBUILD -project Warzone.xcodeproj -target "Fetch Third Party Sources" -PBXBuildsContinueAfterErrors=NO $XCPRETTY; then
		exit ${?}
	fi
fi

exit 0
