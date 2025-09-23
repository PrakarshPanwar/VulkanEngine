# VulkanEngine

I've created my own little Game/Rendering Engine using Vulkan API.

## Getting Started
Visual Studio 2022 is recommended

<ins>1. Downloading the repository</ins>

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

<ins>3. Building Tracy Profiler</ins>
1. Goto [Tracy](VulkanCore/vendor) folder then goto profiler subdirectory.
2. Open the terminal in this directory and make sure you have latest version of CMake installed.
3. Run the following command to build Tracy:
```
cmake -S . -B build
```
4. `-G "Visual Studio 17 2022" -A x64` is optional or just open the `tracy-profiler.sln` file in Visual Studio and build it and skip the next step.
5. Then after building all projects and solution file, type
```
cmake --build build --config Release --target tracy-profiler --parallel
```
This will build the Tracy Profiler Executable. I have also included a Simple Batch Script [TracyLaunchProfiler](TracyLaunchProfiler.bat) to launch the profiler without any hassle. If you don't want tracy profiler just disable `VK_SET_TRACY_PROFILER` in [Core.h](VulkanCore/src/VulkanCore/Core/Core.h)

## Screenshots
1. Main Editor Layer UI
![My Screenshot](EditorLayer/Resources/Screenshots/VulkanEngineUI.png)
2. Full Composited Scene
![My Screenshot](EditorLayer/Resources/Screenshots/VulkanEngineSS.png)

### Big Features to Come
- Advanced UI Features
- Cascaded Shadow Maps