#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script runs autorevision, and tries to download the external libs.
# It attempts the download twice because they may not download properly the first time.
#

##############################
# General Setup

. macosx/BuildBot/_xcodebuild_helpers.sh


##############################
#

cd macosx

# Run autorevision
if ! execute_xcodebuild_command -project Warzone.xcodeproj -target "Autorevision" -configuration "Release"; then
	exit ${?}
fi

# Fetch external libraries
if ! execute_xcodebuild_command -project Warzone.xcodeproj -target "Fetch Third Party Sources"; then
	if ! execute_xcodebuild_command -project Warzone.xcodeproj -target "Fetch Third Party Sources" -PBXBuildsContinueAfterErrors=NO; then
		exstat="${?}"
		echo "ERROR: 2nd attempt to fetch external libraries failed with: ${exstat}"
		exit ${exstat}
	fi
fi

exit 0
