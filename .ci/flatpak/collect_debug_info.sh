#!/bin/bash
# This requires environment variables to be set:
# - WZ_FREEDESKTOP_RUNTIME_VERSION
# - WZ_FLATPAK_TARGET_ARCH

if [ -z "$WZ_FREEDESKTOP_RUNTIME_VERSION" ]; then
  echo "Missing WZ_FREEDESKTOP_RUNTIME_VERSION environment variable"
  exit 1
fi
if [ -z "$WZ_FLATPAK_TARGET_ARCH" ]; then
  echo "Missing WZ_FLATPAK_TARGET_ARCH environment variable"
  exit 1
fi

mkdir output_debug_dependency_snapshot
set +e
# Note: These libraries will be from the runtime that is installed at the time of the build
ldd flatpak_app/files/bin/warzone2100 | grep -E -v 'linux-vdso|ld-linux-' | awk 'NF == 4 { system("echo " $1) }'
ret="$?"
if [ $ret -eq 0 ]; then
  ldd flatpak_app/files/bin/warzone2100 | grep -E -v 'linux-vdso|ld-linux-' | awk 'NF == 4 { system("cp /var/lib/flatpak/runtime/org.freedesktop.Platform/${WZ_FLATPAK_TARGET_ARCH}/${WZ_FREEDESKTOP_RUNTIME_VERSION}/active/files/lib/${WZ_FLATPAK_TARGET_ARCH}-linux-gnu/" $1 " output_debug_dependency_snapshot") }'
else
  # ldd fails for non-native architecture, so use objdump for cross-compiled builds
  objdump -p flatpak_app/files/bin/warzone2100 | grep NEEDED | grep -E -v 'linux-vdso|ld-linux-' | awk 'NF == 2 { system("echo " $2 " && cp /var/lib/flatpak/runtime/org.freedesktop.Platform/${WZ_FLATPAK_TARGET_ARCH}/${WZ_FREEDESKTOP_RUNTIME_VERSION}/active/files/lib/${WZ_FLATPAK_TARGET_ARCH}-linux-gnu/" $2 " output_debug_dependency_snapshot") }'
fi
