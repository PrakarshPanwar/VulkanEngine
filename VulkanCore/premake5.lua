project "VulkanCore"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "Off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "vulkanpch.h"
	pchsource "src/vulkanpch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/glm/glm/**.hpp",
		"vendor/glm/glm/**.inl",
		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"assets/shaders/*.vert",
		"assets/shaders/*.frag",
		"assets/shaders/*.geom"
	}

	includedirs
	{
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.TinyObjLoader}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.stb_image}"
	}

	libdirs { "vendor/VulkanSDK/Lib" }

	links { "GLFW", "vulkan-1.lib", "ImGui" }

	filter "system:windows"
		systemversion "latest"
		defines { "VK_PLATFORM_WINDOWS" }

	filter "configurations:Debug"
		defines { "VK_DEBUG" }
		links { "shaderc_sharedd.lib", "spirv-cross-cored.lib", "spirv-cross-glsld.lib" }
		symbols "On"

	filter "configurations:Release"
		defines { "VK_RELEASE" }
		links { "shaderc_shared.lib", "spirv-cross-core.lib", "spirv-cross-glsl.lib" }
		optimize "On"