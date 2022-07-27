workspace "VulkanCore"
	architecture "x64"
	startproject "VulkanCore"
	configurations { "Debug", "Release" }

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["glm"] = "vendor/glm"
IncludeDir["GLFW"] = "vendor/glfw/include"
IncludeDir["VulkanSDK"] = "vendor/VulkanSDK/Include"
IncludeDir["stb_image"] = "vendor/stb_image"
IncludeDir["TinyObjLoader"] = "vendor/tinyobjloader"
IncludeDir["entt"] = "vendor/entt"
IncludeDir["ImGui"] = "vendor/imgui"

group "Dependencies"
	include "VulkanCore/vendor/glfw"
	include "VulkanCore/vendor/imgui"
group ""

include "VulkanCore"