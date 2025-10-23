#!/bin/sh

# Build Debug Config
/opt/clion/bin/cmake/linux/x64/bin/cmake --build /home/pp2003linux/CLionProjects/VulkanEngine/cmake-build-debug --target all -j 10

# Build Release Config
/opt/clion/bin/cmake/linux/x64/bin/cmake --build /home/pp2003linux/CLionProjects/VulkanEngine/cmake-build-release --target all -j 10
