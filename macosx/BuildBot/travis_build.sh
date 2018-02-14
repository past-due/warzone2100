#!/bin/bash

# IMPORTANT:
# This script is meant to be run from the root of the working copy.
#
# USAGE:
# Execute travis_build.sh with 1-2 parameters
# 1.) Specify one of the valid modes: ("regular", "release") REQUIRED
# 2.) Specify an output path for the created Warzone*.zip files (optional, default: macosx/build/wz_output)
#
# Example:
#	./travis_build.sh nightly "tmp/build_output"
#
# Regular builds:
# - Warzone.zip containing:
#		- Warzone.app built in Release mode with debugging symbols included, but *NO* video sequences
#
# Release builds:
# - Warzone.zip containing:
#		- Warzone.app built in Release mode with debugging symbols included, *AND* video sequences
#

# Handle arguments
if [ -z "$1" ]; then
	echo "travis_build.sh requires an argument specifying one of the valid modes: (\"regular\", \"release\")"
	exit 1
fi
if ! [[ "$1" =~ ^(regular|release)$ ]]; then
	echo "travis_build.sh requires an argument specifying one of the valid modes: (\"regular\", \"release\")"
	exit 1
fi

BUILD_MODE="$1"
OUTPUT_DIR="macosx/build/wz_output"
if [ -n "$2" ]; then
	OUTPUT_DIR="$2"
fi
if [ ! -d "${OUTPUT_DIR}" ]; then
	mkdir -p "${OUTPUT_DIR}"
fi

# Determine whether to fetch & include the videos in the built app bundle
function shouldIncludeVideoSequences {
	local MODE="$1"

	if [ -n "${TRAVIS_PULL_REQUEST}" ] && [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
		# Skip downloading videos for pull requests
		echo "Skip including video sequences for pull requests."
		return 255
	fi
	if [ "${MODE}" != "release" ]; then
		# Skip downloading videos for non-release commits
		echo "Skip including video sequences because input mode is not set to \"release\"."
		return 255
	fi

	true
}

# Clean
echo "macosx/BuildBot/00_clean.sh"
macosx/BuildBot/00_clean.sh
find "${OUTPUT_DIR}" -name "Warzone*.zip" -exec rm -r "{}" \;

# Configure
echo "macosx/BuildBot/01_configure.sh"
if ! macosx/BuildBot/01_configure.sh; then
	exstat="${?}"
	echo "ERROR: 01_configure.sh failed"
	exit ${exstat}
fi

# Fetch the video sequences (if configured)
if shouldIncludeVideoSequences "${BUILD_MODE}"; then
	echo "Fetching video sequences..."
	echo "macosx/BuildBot/02_fetchvideosequences.sh"
	if ! macosx/BuildBot/02_fetchvideosequences.sh; then
		exstat="${?}"
		echo "ERROR: 02_fetchvideosequences.sh failed"
		exit ${exstat}
	fi
fi

# Future TODO: Retrieve the signing certificate from Travis secure data, and configure code-signing

# Compile Warzone.app (and Warzone.zip)
echo "macosx/BuildBot/03_compile.sh"
if ! macosx/BuildBot/03_compile.sh; then
	exstat="${?}"
	echo "ERROR: 03_compile.sh failed"
	exit ${exstat}
fi

# Verify Warzone.zip was created
BUILT_WARZONE_ZIP="macosx/build/Release/Warzone.zip"
if [ ! -f "${BUILT_WARZONE_ZIP}" ]; then
	echo "ERROR: Something went wrong, and Warzone.zip does not exist"
	exit 1
fi

# Collect current working copy Git information
GIT_BRANCH="$(git branch --no-color | sed -e '/^[^*]/d' -e 's:* \(.*\):\1:')"
BUILT_DATETIME=$(date '+%Y%m%d-%H%M%S')
GIT_REVISION_SHORT="$(git rev-parse -q --short --verify HEAD | cut -c1-7)"

# Move Warzone.zip to the output directory, renaming it to:
#  Warzone-{GIT_BRANCH}-{BUILT_DATETIME}-{GIT_REVISION_SHORT}_macOS.zip
DESIRED_ZIP_NAME="Warzone-${GIT_BRANCH}-${BUILT_DATETIME}-${GIT_REVISION_SHORT}_macOS.zip"
if ! mv "$BUILT_WARZONE_ZIP" "${OUTPUT_DIR}/${DESIRED_ZIP_NAME}"; then
	exstat="${?}"
	echo "ERROR: Failed to move zip file"
	exit ${exstat}
fi
echo "Generated Warzone.zip: \"${OUTPUT_DIR}/${DESIRED_ZIP_NAME}\""

exit 0
