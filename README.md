# VulkanEngine

I've created my own little Game/Rendering Engine using Vulkan API.

## Getting Started
Visual Studio 2022 is recommended

<ins>1. Downloading the repository:</ins>

Start by cloning the repository with `git clone --recursive https://github.com/PrakarshPanwar/VulkanEngine`.

1. Run the [VkGenProjects.bat](https://github.com/PrakarshPanwar/VulkanEngine/blob/master/VkGenProjects.bat) file found in main repository.
2. One prerequisite is the Vulkan SDK. If it is not installed, then install [VulkanSDK here](https://vulkan.lunarg.com/).
3. After installation, create a folder `VulkanSDK` and copy all the folders of VulkanSDK in [VulkanCore/vendor/VulkanSDK](https://github.com/PrakarshPanwar/VulkanEngine/tree/master/VulkanCore/vendor).
4. Again run [VkGenProjects.bat](https://github.com/PrakarshPanwar/VulkanEngine/blob/master/VkGenProjects.bat) to link debug libraries in shaderc.

## Issues

1. Need to Add Submodules in Repositories