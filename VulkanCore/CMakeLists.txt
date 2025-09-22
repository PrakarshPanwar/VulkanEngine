# VulkanCore Project Subdirectory
file(GLOB_RECURSE VK_PROJECT_SOURCES
		CONFIGURE_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(spirv_cross_c_shared REQUIRED)

# Add vendor libraries
add_subdirectory(vendor/imgui)
add_subdirectory(vendor/JoltPhysics)
add_subdirectory(vendor/tracy)

# Create library
add_library(VulkanCore
		${VK_PROJECT_SOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_image/stb_image.cpp
	)

# Linking using find_package
target_link_libraries(VulkanCore
		Vulkan::Vulkan
		shaderc_shared
		spirv-cross-core
		ImGui
		glfw
		JoltLib
		assimp
		${YAML_CPP_LIBRARIES}
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/shader-slang/lib/libslang.so
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/tracy/cmake-build-release/libTracyClient.a
	)

target_include_directories(VulkanCore PRIVATE
		${VULKAN_INCLUDE_DIRS}
		${VMA_INCLUDE_DIRS}
		${SPDLOD_INCLUDE_DIRS}
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/entt
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/JoltPhysics
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_image
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGuizmo
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/shader-slang/include
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/tracy/public
	)

# Add Precompiled Headers
target_precompile_headers(VulkanCore PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src/vulkanpch.h
	)

target_compile_definitions(VulkanCore
		PRIVATE IMGUI_DEFINE_MATH_OPERATORS=1
		PUBLIC SPDLOG_USE_STD_FORMAT=1
		PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE=1
		PUBLIC JPH_DEBUG_RENDERER=1
		PUBLIC TRACY_ENABLE
	)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	target_compile_definitions(VulkanCore
			PRIVATE VK_RELEASE=1
		)
endif()
