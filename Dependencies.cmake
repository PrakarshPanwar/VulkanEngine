# Dependencies File
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/VulkanCore/vendor)
set(VULKAN_DIR ${VENDOR_DIR}/VulkanSDK/lib)

# Include Paths
set(GLM_INCLUDE_DIRS ${VENDOR_DIR}/glm)
set(ENTT_INCLUDE_DIRS ${VENDOR_DIR}/entt)
set(STB_INCLUDE_DIRS ${VENDOR_DIR}/stb_image)
set(IMGUI_INCLUDE_DIRS ${VENDOR_DIR}/imgui)
set(IMGUIZMO_INCLUDE_DIRS ${VENDOR_DIR}/ImGuizmo)
set(JOLT_INCLUDE_DIRS ${VENDOR_DIR}/JoltPhysics)
set(TRACY_INCLUDE_DIRS ${VENDOR_DIR}/tracy/public)
set(VULKAN_EXTRA_INCLUDE_DIRS ${VENDOR_DIR}/VulkanSDK/include)
set(MSDF_INCLUDE_DIRS ${VENDOR_DIR}/msdf-atlas-gen)
set(MSDF_GEN_INCLUDE_DIRS ${VENDOR_DIR}/msdf-atlas-gen/msdfgen)

# Libraries
set(SLANG_LIB ${VULKAN_DIR}/libslang.so)
set(SLANG_GLSLANG_LIB ${VULKAN_DIR}/libslang-glslang.so)
set(SPIRV_CROSS_CORE_LIB ${VULKAN_DIR}/libspirv-cross-core.a)
set(SPIRV_CROSS_CPP_LIB ${VULKAN_DIR}/libspirv-cross-cpp.a)
set(SPIRV_CROSS_GLSL_LIB ${VULKAN_DIR}/libspirv-cross-glsl.a)
