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

# Build SDK
flatpak --system install -y --noninteractive flathub org.freedesktop.Sdk//${WZ_FREEDESKTOP_RUNTIME_VERSION}
# Target runtime
flatpak --system install -y --noninteractive flathub org.freedesktop.Platform/${WZ_FLATPAK_TARGET_ARCH}/${WZ_FREEDESKTOP_RUNTIME_VERSION}

if [[ "$WZ_FLATPAK_TARGET_ARCH" != "$WZ_FLATPAK_BUILD_ARCH" ]]; then
  # Cross compiler
  flatpak --system install -y --noninteractive flathub org.freedesktop.Sdk.Extension.toolchain-${WZ_FLATPAK_TARGET_ARCH}//${WZ_FREEDESKTOP_RUNTIME_VERSION}
  # SDK For target runtime
  flatpak --system install -y --noninteractive flathub org.freedesktop.Sdk.Compat.${WZ_FLATPAK_TARGET_ARCH}//${WZ_FREEDESKTOP_RUNTIME_VERSION}
fi

# # Builder (which includes flatpak-builder-lint)
# flatpak install flathub -y --noninteractive org.flatpak.Builder
