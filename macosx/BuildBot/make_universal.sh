#!/bin/sh

# Expected command-line args:
# - Path to each architecture WZ app bundle
#
# This script lipo-s together all of the main executables into a universal binary
# It uses the first WZ app bundle path as the "base" app bundle, and then replaces its main executable with the universal binary
#
# This should likely be followed by re-code-signing (as desired)

set -e

WZ_APP_EXE_PATHS=""

if [ "$#" -le 1 ]; then
  echo "Requires 2 or more arguments"
fi

for WZ_APP_PATH in "$@" ; do
  if [ ! -d "${WZ_APP_PATH}" ]; then
    echo "\"${WZ_APP_PATH}\" is not a directory"
    exit 1
  fi
  WZ_APP_EXECUTABLE_PATH="${WZ_APP_PATH}/Contents/MacOS/Warzone 2100"
  if [ ! -f "${WZ_APP_EXECUTABLE_PATH}" ]; then
    echo "\"${WZ_APP_EXECUTABLE_PATH}\" does not exist"
    exit 1
  fi
  WZ_APP_EXE_PATHS="${WZ_APP_EXE_PATHS} \"${WZ_APP_EXECUTABLE_PATH}\""
done

echo "WZ_APP_EXE_PATHS=${WZ_APP_EXE_PATHS}"

# lipo together all of the single-arch binaries
echo "lipo -create -output \"Warzone 2100-universal\" ${WZ_APP_EXE_PATHS}"
lipo -create -output "Warzone 2100-universal" ${WZ_APP_EXE_PATHS}

# Remove the first app bundle's single-arch binary
BASE_WZ_APP_PATH="$1"
BASE_WZ_APP_EXECUTABLE_PATH="${BASE_WZ_APP_PATH}/Contents/MacOS/Warzone 2100"
rm "${BASE_WZ_APP_EXECUTABLE_PATH}"
echo "mv \"Warzone 2100-universal\" \"${BASE_WZ_APP_EXECUTABLE_PATH}\""
mv "Warzone 2100-universal" "${BASE_WZ_APP_EXECUTABLE_PATH}"

echo "Done."
