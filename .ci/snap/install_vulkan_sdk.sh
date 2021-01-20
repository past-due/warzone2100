#!/bin/bash

TARGET_ARCH="$1"

echo "[install_vulkan_sdk.sh]"
echo "Target Arch: $TARGET_ARCH"

if [ "$TARGET_ARCH" = "amd64_DISABLED" ]; then
  # Install Vulkan SDK (binary package)
  echo "Installing Vulkan SDK"

  # Add Vulkan SDK repo
  wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | apt-key add -
  wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.148-bionic.list https://packages.lunarg.com/vulkan/1.2.148/lunarg-vulkan-1.2.148-bionic.list
  apt update
  apt install --yes vulkan-sdk

else
  # Compile the necessary components of the Vulkan SDK from source
  echo "Compiling Vulkan SDK (selected components)"

  mkdir tmp_vulkan_sdk_build
  cd "tmp_vulkan_sdk_build"

  # glslc
  DEBIAN_FRONTEND=noninteractive apt-get -y cmake python3 lcov

  # glslc - clone
  echo "Fetching shaderc"
  git clone https://github.com/google/shaderc shaderc
  cd shaderc
  ./utils/git-sync-deps
  cd ..

  # glslc - cmake build + install
  mkdir shaderc_build
  cd shaderc_build
  echo "Configuring shaderc"
  cmake -GNinja -DSHADERC_SKIP_TESTS=ON -DCMAKE_BUILD_TYPE=Release ../shaderc/
  echo "Compiling shaderc"
  cmake --build . --target install
  cd ..

  rm -rf "tmp_vulkan_sdk_build"
fi

echo "Installing Vulkan SDK - done"
