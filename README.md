# VulkanEngine

I've created my own little Game/Rendering Engine using Vulkan API.

## Getting Started
Visual Studio 2022 is recommended

<ins>1. Downloading the repository:</ins>

Start by cloning the repository with `git clone --recursive https://github.com/PrakarshPanwar/VulkanEngine.git`.

<ins>2. Setting up Project</ins>
1. Run the [VkGenProjects.bat](VkGenProjects.bat) file found in main repository to generate Project Files.
2. One prerequisite is the Vulkan SDK. If it is not installed, then install [VulkanSDK here](https://vulkan.lunarg.com/).
3. After installation, create a folder **VulkanSDK** in vendor and copy all the folders of VulkanSDK in [VulkanCore/vendor/VulkanSDK](VulkanCore/vendor).
4. Again run [VkGenProjects.bat](VkGenProjects.bat) to link debug libraries in shaderc.

### Big Features to Come

- Post Processing Effects like Bloom, HDR etc.
- Advanced UI Features
- Cascaded Shadow Maps
- HDR Cubemaps and Irradiance Maps
- Physically Based 3D Rendering
- Integrated 3D Physics Engine like [NVIDIA-PhysX](https://github.com/NVIDIAGameWorks/PhysX-3.4), [box2d](https://github.com/erincatto/box2d)