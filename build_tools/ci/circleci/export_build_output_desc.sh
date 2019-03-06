#!/bin/bash

# IMPORTANT:
# This script is meant to be sourced from the root of the working copy.
#
# USAGE:
# Source export_build_output_desc.sh
#
# Example:
#	source export_build_output_desc.sh
#
#
# Copyright © 2018 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

# Determine the build output description prefix (nightly vs tag/release builds)
# For nightly builds, base on the autorevision info: <BRANCH>-
# For tags/releases, base on the tag name: <TAG_NAME>-
if [ -n "${CIRCLE_TAG}" ] && [ -z "${CIRCLE_PULL_REQUEST}" ]; then
	# Replace "/" so the CIRCLE_TAG can be used in a filename
	CIRCLE_TAG_SANITIZED="$(echo "${CIRCLE_TAG}" | sed -e 's:/:_:' -e 's:-:_:')"
	export WZ_BUILD_DESC_PREFIX="${CIRCLE_TAG_SANITIZED}"
else
	# Collect current working copy Git information
	GIT_BRANCH="$(git branch --no-color | sed -e '/^[^*]/d' -e 's:* \(.*\):\1:')"

	if [ -n "${CIRCLE_PULL_REQUEST}" ]; then
		if [ -n "${CIRCLE_PR_NUMBER}" ]; then
			echo "Triggered by a Pull Request - use the CIRCLE_PR_NUMBER (${CIRCLE_PR_NUMBER}) for the output filename"
			GIT_BRANCH="PR_#${CIRCLE_PR_NUMBER}"
		else
			echo "Triggered by a Pull Request - use the CIRCLE_PULL_REQUEST (${CIRCLE_PULL_REQUEST}) for the output filename"
			GIT_BRANCH="PR_${CIRCLE_PULL_REQUEST}"
		fi
	elif [ -n "${CIRCLE_BRANCH}" ]; then
		echo "Use the CIRCLE_BRANCH (${CIRCLE_BRANCH}) as the branch for the output filename"
		GIT_BRANCH="${CIRCLE_BRANCH}"
	fi

	# Replace "/" so the GIT_BRANCH can be used in a filename
	GIT_BRANCH_SANITIZED="$(echo "${GIT_BRANCH}" | sed -e 's:/:_:' -e 's:-:_:')"
	export WZ_BUILD_DESC_PREFIX="${GIT_BRANCH_SANITIZED}"
fi

echo "WZ_BUILD_DESC_PREFIX=${WZ_BUILD_DESC_PREFIX}"
