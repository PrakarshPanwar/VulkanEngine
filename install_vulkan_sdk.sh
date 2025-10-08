#!/bin/bash
set -e

# Get Latest SDK Version
VULKAN_SDK_VERSION=`curl https://vulkan.lunarg.com/sdk/latest/linux.txt`
echo "Vulkan SDK Latest Version: ${VULKAN_SDK_VERSION}"

# Goto vendor directory
cd VulkanCore/vendor

# Check if VulkanSDK folder exists
if [ ! -d "./VulkanSDK" ]; then
	echo "Creating VulkanSDK Folder"
	mkdir VulkanSDK
fi

# Install Vulkan SDK Tarball(if not installed)
if [ ! -f "$HOME/Downloads/vulkan_sdk.tar.xz" ]; then
	echo "Installing VulkanSDK..."
	wget https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/linux/vulkan_sdk.tar.xz -P ~/Downloads
else
	echo "VulkanSDK(${VULKAN_SDK_VERSION}) is already installed"
fi

# Extract specific directories/files from Tarball
# Libraries(spirv_cross, slang, vma)
# Bin(vulkanCapsViewer, vkconfig, vkconfig-gui)
cd VulkanSDK
sed "s|^|${VULKAN_SDK_VERSION}|"  ../../../vulkan_sdk_configs.txt > vulkan_sdk_config_paths.txt
tar -xvf ~/Downloads/vulkan_sdk.tar.xz -T vulkan_sdk_config_paths.txt --strip-components=2 
