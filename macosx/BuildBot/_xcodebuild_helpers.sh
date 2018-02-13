#!/bin/bash

# This script is included by the other scripts in this directory.
#

##############################
# General Setup

XCODEBUILD="xcodebuild"

# Determine if xcpretty is present
XCPRETTY_PATH=$(command -v xcpretty 2> /dev/null)

# If xcpretty is present, use it to format xcodebuild output
XCPRETTY=
if [ -n "$XCPRETTY_PATH" ]; then
	XCPRETTY="xcpretty -c"

	# On Travis-CI, use xcpretty-travis-formatter
	if [ -n "$TRAVIS" ]; then
		XCPRETTY="$XCPRETTY -f `xcpretty-travis-formatter`"
	fi
fi


##############################
# Helper Functions

execute_xcodebuild_command () {
set -x
	if [ -n "$XCPRETTY" ]; then
		set -o pipefail && $XCODEBUILD "$@" | $XCPRETTY
		return $?
	else
		$XCODEBUILD "$@"
		return $?
	fi
}
