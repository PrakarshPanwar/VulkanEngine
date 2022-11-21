project "EditorLayer"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "Off"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files {
		"src/**.cpp",
		"src/**.h"
	}

	includedirs {
		"%{wks.location}/VulkanCore/vendor/spdlog/include",
		"%{wks.location}/VulkanCore/src",
		"%{IncludeDir.glm}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.GLAD}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.VulkanSDK}",
		"%{IncludeDir.Assimp}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}"
	}

	links { "VulkanCore" }

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "VK_DEBUG"
		runtime "Debug"
		symbols "on"

		postbuildcommands { "{COPY} %{Library.AssimpDLLDebug} ../bin/" .. outputdir .. "/%{prj.name}" }

	filter "configurations:Release"
		defines "VK_RELEASE"
		runtime "Release"
		optimize "on"

		postbuildcommands { "{COPY} %{Library.AssimpDLLRelease} ../bin/" .. outputdir .. "/%{prj.name}" }