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

execute_xcodebuild_command  \
 -project Warzone.xcodeproj \
 -target "Warzone" \
 -configuration "Release" \
 -destination "platform=macOS" \
 -PBXBuildsContinueAfterErrors=NO
result=${?}
if [ $result -ne 0 ]; then
	exit ${result}
fi

# Create Warzone.zip
cd build/Release

if [ ! -d "Warzone.app" ]; then
	echo "ERROR: Warzone.app is not present in build/Release"
	exit 1
fi

# For debugging purposes, verify & output some information about the generated Warzone.app
echo "Generated Warzone.app"
generated_infoplist_location="Warzone.app/Contents/Info.plist"
generated_buildnumber=$(/usr/libexec/PlistBuddy -c "Print CFBundleVersion" "${generated_infoplist_location}")
echo "  -> Build Number (CFBundleVersion): ${generated_buildnumber}"
generated_versionnumber=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "${generated_infoplist_location}")
echo "  -> Version Number (CFBundleShortVersionString): ${generated_versionnumber}"

echo "Creating Warzone.zip..."
if [ -n "$TRAVIS" ]; then
	# On Travis-CI, emit fold indicators
	echo "travis_fold:start:zip.warzone"
fi
if ! zip -r Warzone.zip -qdgds 10m Warzone.app; then
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
