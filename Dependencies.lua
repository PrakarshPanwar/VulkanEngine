
-- VulkanCore Dependencies

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/VulkanCore/vendor/glfw/include"
IncludeDir["stb_image"] = "%{wks.location}/VulkanCore/vendor/stb_image"
IncludeDir["entt"] = "%{wks.location}/VulkanCore/vendor/entt"
IncludeDir["glm"] = "%{wks.location}/VulkanCore/vendor/glm"
IncludeDir["ImGui"] = "%{wks.location}/VulkanCore/vendor/imgui"
IncludeDir["ImGuizmo"] = "%{wks.location}/VulkanCore/vendor/ImGuizmo"
IncludeDir["yaml_cpp"] = "%{wks.location}/VulkanCore/vendor/yaml-cpp/include"
IncludeDir["optick"] = "%{wks.location}/VulkanCore/vendor/Optick/include"
IncludeDir["TinyObjLoader"] = "%{wks.location}/VulkanCore/vendor/tinyobjloader"
IncludeDir["VulkanSDK"] = "%{wks.location}/VulkanCore/vendor/VulkanSDK/Include"
IncludeDir["Assimp"] = "%{wks.location}/VulkanCore/vendor/Assimp/include"
IncludeDir["Jolt"] = "%{wks.location}/VulkanCore/vendor/JoltPhysics"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{wks.location}/VulkanCore/vendor/VulkanSDK/Lib"
LibraryDir["Assimp"] = "%{wks.location}/VulkanCore/vendor/Assimp/lib"
LibraryDir["AssimpBin"] = "%{wks.location}/VulkanCore/vendor/Assimp/bin"
LibraryDir["optick"] = "%{wks.location}/VulkanCore/vendor/Optick/lib/x64"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"
Library["Slang_Debug"] = "%{LibraryDir.VulkanSDK}/slangd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"
Library["Slang_Release"] = "%{LibraryDir.VulkanSDK}/slang.lib"

Library["optick_Debug"] = "%{LibraryDir.optick}/debug/OptickCore.lib"
Library["optick_Release"] = "%{LibraryDir.optick}/release/OptickCore.lib"
Library["optick_DLL_Debug"] = "%{LibraryDir.optick}/debug/OptickCore.dll"
Library["optick_DLL_Release"] = "%{LibraryDir.optick}/release/OptickCore.dll"

Library["AssimpLibDebug"] = "%{LibraryDir.Assimp}/Debug/assimp-vc143-mtd.lib"
Library["AssimpZlibDebug"] = "%{LibraryDir.Assimp}/Debug/zlibd.lib"

Library["AssimpLibRelease"] = "%{LibraryDir.Assimp}/Release/assimp-vc143-mt.lib"
Library["AssimpZlibRelease"] = "%{LibraryDir.Assimp}/Release/zlib.lib"

Library["AssimpDLLDebug"] = "%{LibraryDir.AssimpBin}/Debug/assimp-vc143-mtd.dll"
Library["AssimpDLLRelease"] = "%{LibraryDir.AssimpBin}/Release/assimp-vc143-mt.dll"

Library["ZlibDLLDebug"] = "%{LibraryDir.AssimpBin}/Debug/zlibd.dll"
Library["ZlibDLLRelease"] = "%{LibraryDir.AssimpBin}/Release/zlib.dll"