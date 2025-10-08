# Dependencies File
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/VulkanCore/vendor)

# Include Paths
set(GLM_INCLUDE_DIRS ${VENDOR_DIR}/glm)
set(ENTT_INCLUDE_DIRS ${VENDOR_DIR}/entt)
set(STB_INCLUDE_DIRS ${VENDOR_DIR}/stb_image)
set(IMGUI_INCLUDE_DIRS ${VENDOR_DIR}/imgui)
set(IMGUIZMO_INCLUDE_DIRS ${VENDOR_DIR}/ImGuizmo)
set(JOLT_INCLUDE_DIRS ${VENDOR_DIR}/JoltPhysics)
set(TRACY_INCLUDE_DIRS ${VENDOR_DIR}/tracy/public)
set(VULKAN_EXTRA_INCLUDE_DIRS ${VENDOR_DIR}/VulkanSDK/include)

# Libraries
set(SLANG_LIB ${VENDOR_DIR}/VulkanSDK/lib/libslang.so)
set(SLANG_GLSLANG_LIB ${VENDOR_DIR}/VulkanSDK/lib/libslang-glslang.so)
set(SPIRV_CROSS_CORE_LIB ${VENDOR_DIR}/VulkanSDK/lib/libspirv-cross-core.a)
set(SPIRV_CROSS_CPP_LIB ${VENDOR_DIR}/VulkanSDK/lib/libspirv-cross-cpp.a)
set(SPIRV_CROSS_GLSL_LIB ${VENDOR_DIR}/VulkanSDK/lib/libspirv-cross-glsl.a)
