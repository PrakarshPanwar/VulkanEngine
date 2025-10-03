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
		${ENTT_INCLUDE_DIRS}
		${GLM_INCLUDE_DIRS}
		${IMGUI_INCLUDE_DIRS}
		${IMGUIZMO_INCLUDE_DIRS}
		${TRACY_INCLUDE_DIRS}
	)

# Link VulkanCore Library
target_link_libraries(EditorLayer PRIVATE VulkanCore)

# Enable SIMD flags
target_compile_options(EditorLayer PRIVATE -msse4.1)
