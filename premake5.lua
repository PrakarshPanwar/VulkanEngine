workspace "VulkanCore"
	architecture "x64"
	startproject "EditorLayer"
	configurations { "Debug", "Release" }

	flags { "MultiProcessorCompile" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Dependencies.lua"

group "Dependencies"
	include "VulkanCore/vendor/glfw"
	include "VulkanCore/vendor/imgui"
group ""

include "VulkanCore"
include "EditorLayer"