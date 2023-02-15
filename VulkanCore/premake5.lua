project "VulkanCore"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "Off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "vulkanpch.h"
	pchsource "src/vulkanpch.cpp"

	files {
		"src/**.h",
		"src/**.cpp",
		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",
		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"assets/shaders/*.vert",
		"assets/shaders/*.frag",
		"assets/shaders/*.geom",
		"assets/shaders/*.comp"
	}

	removefiles {
		"src/Platform/Vulkan/VulkanGameObject.h",
		"src/Platform/Vulkan/VulkanGameObject.cpp",
		"src/VulkanCore/Core/FrameInfo.h"
	}

	includedirs {
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.TinyObjLoader}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.Assimp}"
	}

	links { "GLFW", "%{Library.Vulkan}", "ImGui", "yaml-cpp" }

	filter "system:windows"
		systemversion "latest"
		defines { "VK_PLATFORM_WINDOWS" }

	filter "configurations:Debug"
		defines { "VK_DEBUG" }
		links { "%{Library.ShaderC_Debug}", "%{Library.SPIRV_Cross_Debug}", "%{Library.SPIRV_Cross_GLSL_Debug}", "%{Library.AssimpLibDebug}" }
		symbols "On"

		postbuildcommands { "{COPY} %{Library.AssimpDLLDebug} ../bin/" .. outputdir .. "/%{prj.name}" }

	filter "configurations:Release"
		defines { "VK_RELEASE" }
		links { "%{Library.ShaderC_Release}", "%{Library.SPIRV_Cross_Release}", "%{Library.SPIRV_Cross_GLSL_Release}", "%{Library.AssimpLibRelease}" }
		optimize "On"

		postbuildcommands { "{COPY} %{Library.AssimpDLLRelease} ../bin/" .. outputdir .. "/%{prj.name}" }
