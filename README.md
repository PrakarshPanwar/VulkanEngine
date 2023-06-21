# VulkanEngine

I've created my own little Game/Rendering Engine using Vulkan API.

## Getting Started
Visual Studio 2022 is recommended

<ins>1. Downloading the repository:</ins>

Start by cloning the repository with `git clone --recursive https://github.com/PrakarshPanwar/VulkanEngine.git`.

<ins>2. Setting up Project</ins>
1. Run the [VkGenProjects.bat](VkGenProjects.bat) file found in main repository to generate Project Files.
2. Build Assimp submodule by typing below text in Command Prompt
```
cd Assimp 
cmake -DASSIMP_BUILD_ZLIB=ON CMakeLists.txt
```
3. Open `Assimp.sln` in Visual Studio and build solution.
4. One prerequisite is the Vulkan SDK. If it is not installed, then install [VulkanSDK here](https://vulkan.lunarg.com/).
5. After installation, create a folder **VulkanSDK** in vendor and copy all the folders of VulkanSDK in [VulkanCore/vendor/VulkanSDK](VulkanCore/vendor).
6. Again run [VkGenProjects.bat](VkGenProjects.bat) to link debug libraries in shaderc.

### Big Features to Come

- Advanced UI Features
- Cascaded Shadow Maps
- Integrated 3D Physics Engine like [NVIDIA-PhysX](https://github.com/NVIDIAGameWorks/PhysX), [box2d](https://github.com/erincatto/box2d)