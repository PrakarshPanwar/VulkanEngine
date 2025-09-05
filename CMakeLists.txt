cmake_minimum_required(VERSION 3.60)
project(VulkanEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(VulkanCore)
add_subdirectory(EditorLayer)
