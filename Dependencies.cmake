# Dependencies File
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/VulkanCore/vendor)

# Include Paths
set(GLM_INCLUDE_DIRS ${VENDOR_DIR}/glm)
set(ENTT_INCLUDE_DIRS ${VENDOR_DIR}/entt)
set(STB_INCLUDE_DIRS ${VENDOR_DIR}/stb_image)
set(IMGUI_INCLUDE_DIRS ${VENDOR_DIR}/imgui)
set(IMGUIZMO_INCLUDE_DIRS ${VENDOR_DIR}/ImGuizmo)
set(JOLT_INCLUDE_DIRS ${VENDOR_DIR}/JoltPhysics)
set(SLANG_INCLUDE_DIRS ${VENDOR_DIR}/shader-slang/include)
set(TRACY_INCLUDE_DIRS ${VENDOR_DIR}/tracy/public)

# Libraries
set(SLANG_LIB ${VENDOR_DIR}/shader-slang/lib/libslang.so)
