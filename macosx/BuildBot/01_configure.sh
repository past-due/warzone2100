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
execute_xcodebuild_command -project Warzone.xcodeproj -target "Autorevision" -configuration "Release"
result=${?}
if [ $result -ne 0 ]; then
	exit ${result}
fi

# FOR DEBUGGING
# Output the values from autorevision
. "../src/autorevision.cache"
echo "Generated autorevision.cache info..."
echo "VCS_TYPE=${VCS_TYPE}"
echo "VCS_BASENAME=${VCS_BASENAME}"
echo "VCS_BRANCH=${VCS_BRANCH}"
echo "VCS_TAG=${VCS_TAG}"
echo "VCS_EXTRA=${VCS_EXTRA}"
echo "VCS_FULL_HASH=${VCS_FULL_HASH}"
echo "VCS_SHORT_HASH=${VCS_SHORT_HASH}"
echo "VCS_WC_MODIFIED=${VCS_WC_MODIFIED}"
echo ""
echo "Additional info:"
echo "git rev-list --count HEAD => \"$(git rev-list --count HEAD)\""
echo "git describe --abbrev=0 --tags 2> /dev/null => \"$(git describe --abbrev=0 --tags 2> /dev/null)\""
echo "."

# See: https://stackoverflow.com/a/44036486
function create_all_branches()
{
    # Keep track of where Travis put us.
    # We are on a detached head, and we need to be able to go back to it.
    local build_head=$(git rev-parse HEAD)

    # Fetch all the remote branches. Travis clones with `--depth`, which
    # implies `--single-branch`, so we need to overwrite remote.origin.fetch to
    # do that.
    git config --replace-all remote.origin.fetch +refs/heads/*:refs/remotes/origin/*
    git fetch
    # optionally, we can also fetch the tags
    git fetch --tags

#    # create the tacking branches
#    for branch in $(git branch -r|grep -v HEAD) ; do
#        git checkout -qf ${branch#origin/}
#    done

    # finally, go back to where we were at the beginning
    git checkout ${build_head}
}

create_all_branches
echo "Additional info [2]:"
echo "git rev-list --count HEAD => \"$(git rev-list --count HEAD)\""
echo "git describe --abbrev=0 --tags 2> /dev/null => \"$(git describe --abbrev=0 --tags 2> /dev/null)\""
echo "."


# Fetch external libraries
if ! execute_xcodebuild_command -project Warzone.xcodeproj -target "Fetch Third Party Sources"; then
	execute_xcodebuild_command -project Warzone.xcodeproj -target "Fetch Third Party Sources" -PBXBuildsContinueAfterErrors=NO
	result=${?}
	if [ $result -ne 0 ]; then
		echo "ERROR: 2nd attempt to fetch external libraries failed with: ${result}"
		exit ${result}
	fi
fi

exit 0
