
-- VulkanCore Dependencies

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/VulkanCore/vendor/glfw/include"
IncludeDir["stb_image"] = "%{wks.location}/VulkanCore/vendor/stb_image"
IncludeDir["entt"] = "%{wks.location}/VulkanCore/vendor/entt"
IncludeDir["glm"] = "%{wks.location}/VulkanCore/vendor/glm"
IncludeDir["ImGui"] = "%{wks.location}/VulkanCore/vendor/imgui"
IncludeDir["TinyObjLoader"] = "%{wks.location}/VulkanCore/vendor/tinyobjloader"
IncludeDir["VulkanSDK"] = "%{wks.location}/VulkanCore/vendor/VulkanSDK/Include"
IncludeDir["Assimp"] = "%{wks.location}/VulkanCore/vendor/Assimp/include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{wks.location}/VulkanCore/vendor/VulkanSDK/Lib"
LibraryDir["Assimp"] = "%{wks.location}/VulkanCore/vendor/Assimp/lib"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

Library["AssimpLibDebug"] = "%{LibraryDir.Assimp}/Debug/assimp-vc143-mtd.lib"
Library["AssimpLibRelease"] = "%{LibraryDir.Assimp}/Release/assimp-vc143-mt.lib"

Library["AssimpZlibDebug"] = "%{LibraryDir.Assimp}/Debug/zlibd.lib"
Library["AssimpZlibRelease"] = "%{LibraryDir.Assimp}/Release/zlib.lib"