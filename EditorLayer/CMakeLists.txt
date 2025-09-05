# EditorLayer Project Subdirectory
file(GLOB_RECURSE EL_PROJECT_SOURCES
		CONFIGURE_DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	)

# Create executable
add_executable(EditorLayer ${EL_PROJECT_SOURCES})

# Include directories
target_include_directories(EditorLayer PRIVATE
#		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_SOURCE_DIR}/VulkanCore/src
		${CMAKE_SOURCE_DIR}/VulkanCore/vendor/entt
		${CMAKE_SOURCE_DIR}/VulkanCore/vendor/glm
	)
