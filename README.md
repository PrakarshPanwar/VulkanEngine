# VulkanEngine

I've created my own little Game/Rendering Engine using Vulkan API.

## Getting Started
Visual Studio 2022 is recommended

<ins>1. Downloading the repository:</ins>

Start by cloning the repository with `git clone --recursive https://github.com/PrakarshPanwar/VulkanEngine.git`.

<ins>2. Setting up Project</ins>
1. Run the [VkGenProjects.bat](VkGenProjects.bat) file found in main repository to generate Project Files.
2. Build Assimp submodule by typing `Y` after this shows up in Command Prompt after Projects Build.
```
BUILD ASSIMP SOLUTION(Y/N)=
```
3. One prerequisite is the Vulkan SDK. If it is not installed, then install [VulkanSDK here](https://vulkan.lunarg.com/).
4. During SDK Setup, check Vulkan Memory Allocator Header and Shader Toolchain Debug, then proceed with installation. 
5. After installation, create a folder **VulkanSDK** in VulkanCore/vendor and copy all the folders of VulkanSDK in [VulkanCore/vendor/VulkanSDK](VulkanCore/vendor).
6. Again run [VkGenProjects.bat](VkGenProjects.bat) to link debug libraries in shaderc.

## Screenshots
1. Main Editor Layer UI
![My Screenshot](EditorLayer/Resources/Screenshots/VulkanEngineUI.png)
2. Full Composited Scene
![My Screenshot](EditorLayer/Resources/Screenshots/VulkanEngineSS.png)

### Big Features to Come
- Advanced UI Features
- Cascaded Shadow Maps