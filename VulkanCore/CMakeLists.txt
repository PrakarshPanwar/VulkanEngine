# VulkanCore Project Subdirectory
file(GLOB_RECURSE VK_PROJECT_SOURCES
		CONFIGURE_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	)

find_package(Vulkan REQUIRED)
find_package(yaml-cpp REQUIRED)

# Add vendor libraries
add_subdirectory(vendor/imgui)
add_subdirectory(vendor/JoltPhysics)
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/msdf-atlas-gen)

# Create library
add_library(VulkanCore
		${VK_PROJECT_SOURCES}
		${VENDOR_DIR}/stb_image/stb_image.cpp
		${VENDOR_DIR}/tracy/public/TracyClient.cpp
	)

# Linking using find_package
target_link_libraries(VulkanCore
		Vulkan::Vulkan
		shaderc_shared
		ImGui
		glfw
		JoltLib
		assimp
		msdf-atlas-gen
		${YAML_CPP_LIBRARIES}
		${SLANG_LIB}
		${SLANG_GLSLANG_LIB}
		${SPIRV_CROSS_CORE_LIB}
		${SPIRV_CROSS_CPP_LIB}
		${SPIRV_CROSS_GLSL_LIB}
	)

target_include_directories(VulkanCore PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${VULKAN_INCLUDE_DIRS}
		${VULKAN_EXTRA_INCLUDE_DIRS}
		${VMA_INCLUDE_DIRS}
		${SPDLOD_INCLUDE_DIRS}
		${ENTT_INCLUDE_DIRS}
		${GLM_INCLUDE_DIRS}
		${JOLT_INCLUDE_DIRS}
		${STB_INCLUDE_DIRS}
		${IMGUI_INCLUDE_DIRS}
		${IMGUIZMO_INCLUDE_DIRS}
		${SLANG_INCLUDE_DIRS}
		${TRACY_INCLUDE_DIRS}
		${MSDF_INCLUDE_DIRS}
		${MSDF_GEN_INCLUDE_DIRS}
	)

# Add Precompiled Headers
target_precompile_headers(VulkanCore PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src/vulkanpch.h
	)

target_compile_definitions(VulkanCore
		PRIVATE IMGUI_DEFINE_MATH_OPERATORS
		PUBLIC SPDLOG_USE_STD_FORMAT
		PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE
		PUBLIC JPH_DEBUG_RENDERER
		PUBLIC TRACY_ENABLE
		PUBLIC MSDFGEN_PUBLIC=
		PUBLIC FRAMES_IN_FLIGHT=2
	)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	target_compile_definitions(VulkanCore
			PRIVATE VK_RELEASE
		)
endif()
