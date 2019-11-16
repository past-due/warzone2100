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
# Copyright © 2018-2019 pastdue ( https://github.com/past-due/ ) and contributors
# License: MIT License ( https://opensource.org/licenses/MIT )
#

if [ -z "${GITHUB_REF}" ]; then
	echo "Missing expected GITHUB_REF environment variable"
	exit 1
fi

# Extract branch / tag from GITHUB_REF
# (examples: GITHUB_REF=refs/heads/master, GITHUB_REF=refs/tags/v3.3.0, GITHUB_REF=refs/pull/3/merge (for a pull_request event))
ref_tmp=${GITHUB_REF#*/} ## throw away the first part of the ref
ref_type=${ref_tmp%%/*} ## extract the second element of the ref (heads or tags)
ref_value=${ref_tmp#*/} ## extract the third+ elements of the ref (master or v3.3.0)

# For tags/releases, base on the tag name
if [ "$ref_type" == "tags" ]; then
	# Replace "/" so the tag can be used in a filename
	TAG_SANITIZED="$(echo "${ref_value}" | sed -e 's:/:_:g' -e 's:-:_:g')"
	export WZ_BUILD_DESC_PREFIX="${TAG_SANITIZED}"
else
	GIT_BRANCH="${ref_value}"

	if [ -n "${GITHUB_HEAD_REF}" ]; then
		# Use the head ref's branch name
		GIT_BRANCH="${GITHUB_HEAD_REF}"
	fi

	# Replace "/" so the GIT_BRANCH can be used in a filename
	GIT_BRANCH_SANITIZED="$(echo "${GIT_BRANCH}" | sed -e 's:/:_:g' -e 's:-:_:g')"
	export WZ_BUILD_DESC_PREFIX="${GIT_BRANCH_SANITIZED}"
fi

echo "WZ_BUILD_DESC_PREFIX=${WZ_BUILD_DESC_PREFIX}"
