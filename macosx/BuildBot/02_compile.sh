#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script builds `Warzone.app` and the `Warzone.app.dSYM` bundle, and combines them into `Warzone.zip`.
#
# (It is very important to keep the dSYM bundle with its .app; it contains irreplaceable debug info.)
#

##############################
# General Setup

. macosx/BuildBot/_xcodebuild_helpers.sh


##############################
# Build Warzone

cd macosx

if ! execute_xcodebuild_command  \
 -project Warzone.xcodeproj \
 -target "Warzone" \
 -configuration "Release" \
 -destination "platform=macOS" \
 -PBXBuildsContinueAfterErrors=NO; then
	exit ${?}
fi

# Create Warzone.zip
cd build/Release

echo "Creating Warzone.zip..."
if [ -n "$TRAVIS" ]; then
	# On Travis-CI, emit fold indicators
	echo "travis_fold:start:zip.warzone"
fi
if ! zip -r Warzone.zip Warzone.app Warzone.app.dSYM; then
	exit ${?}
fi
if [ -n "$TRAVIS" ]; then
	# On Travis-CI, emit fold indicators
	echo "travis_fold:end:zip.warzone"
fi
echo "Warzone.zip created."
cd ../..

cd ..

exit ${?}
