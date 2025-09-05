# VulkanCore Project Subdirectory
file(GLOB_RECURSE VK_PROJECT_SOURCES
		CONFIGURE_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	)

# Create executable
add_library(VulkanCore ${VK_PROJECT_SOURCES})

# Linking using find_package(Vulkan)
target_link_libraries(VulkanCore ${VULKAN_LIBRARIES})
target_include_directories(VulkanCore PRIVATE
		${VULKAN_INCLUDE_DIRS}
		${VMA_INCLUDE_DIRS}
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/entt
		${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
	)

# Add Precompiled Headers
target_precompile_headers(VulkanCore PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src/vulkanpch.h"
	)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	target_compile_definitions(VulkanCore
			PRIVATE VK_RELEASE=1
		)
endif()
