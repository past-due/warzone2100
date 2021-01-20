#!/bin/bash

TARGET_ARCH="$1"

echo "[install_vulkan_sdk.sh]"
echo "Target Arch: $TARGET_ARCH"

# Add Vulkan SDK repo
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add -
wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.148-bionic.list https://packages.lunarg.com/vulkan/1.2.148/lunarg-vulkan-1.2.148-bionic.list

if [ "$TARGET_ARCH" = "amd64_DISABLED" ]; then
  # Install Vulkan SDK (binary package)
  echo "Installing Vulkan SDK"

  apt update
  apt install --yes vulkan-sdk
else
  # Compile Vulkan SDK from source
  echo "Compiling Vulkan SDK"

  mkdir tmp_vulkan_sdk_build
  cd "tmp_vulkan_sdk_build"

  apt-src update
  apt-src --build install vulkan-sdk

  cd ..
  rm -rf "tmp_vulkan_sdk_build"
fi

echo "Installing Vulkan SDK - done"
