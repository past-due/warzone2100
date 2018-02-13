#!/bin/bash

# Note:
# This script is meant to be run from the root of the working copy.
# 
# This script runs autorevision, and tries to download the external libs and sequences.
# It attempts the download twice because they may not download properly the first time.
#
# IMPORTANT:
# To download the sequences, the environment variable "MAC_BUILD_RELEASE" must be set to "true"
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
		exit ${?}
	fi
fi

. "../src/autorevision.cache"
if [[ ! "${VCS_TAG}" == *_beta* ]] && [[ ! "${VCS_TAG}" == *_rc* ]]; then
	if [ -n "$TRAVIS_PULL_REQUEST" ] && [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
		# Skip downloading videos for pull requests
		echo "Skip including video sequences for pull requests."
		exit 0
	fi
	if [ "$MAC_BUILD_RELEASE" != "true" ]; then
		# Skip downloading videos for non-release commits
		echo "Skip including video sequences because MAC_BUILD_RELEASE is not set to \"true\"."
		exit 0
	fi

	# Should fetch video sequences
	# For now, specify the standard quality sequence only
	# And download it directly into the ../data/ folder, so it gets picked up by the Xcode app build
	if ! configs/FetchVideoSequences.sh standard ../data; then
		exit ${?}
	fi
fi


exit 0
