# VulkanEngine

I've created my own little Game/Rendering Engine using Vulkan API.

## Getting Started
**Windows** - Visual Studio 2022 is recommended

**Linux** - CLion IDE is recommended

<ins>1. Downloading the repository</ins>

Start by cloning the repository with `git clone --recursive https://github.com/PrakarshPanwar/VulkanEngine.git`.

<ins>2. Setting up Project</ins>
### Windows
1. Run the [VkGenProjects.bat](VkGenProjects.bat) file found in main repository to generate Project Files.
2. Build Assimp submodule by typing `Y` after this shows up in Command Prompt after Projects Build.
```
BUILD ASSIMP SOLUTION(Y/N):
```
3. Build Tracy Profiler(optional) by typing `Y` after Assimp binaries are built
```
BUILD TRACY PROFILER SOLUTION(Y/N):
```
4. One prerequisite is the Vulkan SDK. If it is not installed, then install [VulkanSDK here](https://vulkan.lunarg.com/).
5. During SDK Setup, check Vulkan Memory Allocator Header and Shader Toolchain Debug, then proceed with installation. 
6. After installation, create a folder **VulkanSDK** in VulkanCore/vendor and copy all the folders of VulkanSDK in [VulkanCore/vendor/VulkanSDK](VulkanCore/vendor).
7. Again run [VkGenProjects.bat](VkGenProjects.bat) to link debug libraries in shaderc.

### Linux
1. Checkout **linux/main** branch.
2. If you are using CLion IDE, it will automatically detect CMake files, right click on root [CMakeLists.txt](CMakeLists.txt) and click **Reload CMake Project**.
3. Install following packages

Fedora/RHEL Distributions
```
sudo dnf install vulkan-loader-devel vulkan-validation-layers vulkan-tools glfw-devel assimp-devel spdlog-devel yaml-cpp-devel libshaderc-devel
```
Ubuntu/Debian Distributions(Not tested)
```
sudo apt-get install libvulkan-dev vulkan-validationlayers vulkan-tools libglfw3-dev libassimp-dev libspdlog-dev libyaml-cpp-dev libshaderc-dev
```
These packages are provided by default on most mainstream distros.

4. Give shell scripts permission to execute
```
chmod +x run_editor.sh run_tracy.sh run_gdb.sh install_vulkan_sdk.sh
```
5. Execute `install_vulkan_sdk.sh` and wait for additional Vulkan dependencies to install.

<ins>3. Building Tracy Profiler</ins>

### Windows
1. It is optional to build it in [VkGenProjects.bat](VkGenProjects.bat).
2. To run it, execute [TracyLaunchProfiler](TracyLaunchProfiler.bat).

### Linux
1. Make sure to install these packages

Fedora/RHEL Distributions
```
sudo dnf install patch dbus-devel wayland-devel
```
Ubuntu/Debian Distributions(Not tested)
```
sudo apt-get install patch libdbus-1-dev libwayland-dev
```

2. Run the following commands to build Tracy
```
cmake -S . -B build
cmake --build build --config Release --target tracy-profiler --parallel
```

3. To run it, execute [run_tracy.sh](run_tracy.sh).

## Screenshots
1. Main Editor Layer UI
![My Screenshot](EditorLayer/Resources/Screenshots/VulkanEngineUI.png)
2. Full Composited Scene
![My Screenshot](EditorLayer/Resources/Screenshots/VulkanEngineSS.png)

### Features to Add
- Advanced UI Features
- Cascaded Shadow Maps
