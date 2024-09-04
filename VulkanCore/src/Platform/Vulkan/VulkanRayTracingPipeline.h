#pragma once
#include "VulkanContext.h"
#include "VulkanDescriptor.h"
#include "VulkanCore/Renderer/RayTracingPipeline.h"
#include "Platform/Vulkan/VulkanShaderBindingTable.h"
#include "Platform/Vulkan/VulkanRayTraceShader.h"

namespace VulkanCore {

	class VulkanRayTracingPipeline : public RayTracingPipeline
	{
	public:
		VulkanRayTracingPipeline() = default;
		VulkanRayTracingPipeline(std::shared_ptr<ShaderBindingTable> shaderBindingTable, const std::string& debugName);
		~VulkanRayTracingPipeline();

		void Release();

		void Bind(VkCommandBuffer commandBuffer);
		void SetPushConstants(VkCommandBuffer cmdBuf, void* pcData, size_t size);
		void ReloadPipeline() override;

		inline std::shared_ptr<Shader> GetShader() const override { return m_ShaderBindingTable->GetShader(); }
		inline VkPipelineLayout GetVulkanPipelineLayout() const { return m_PipelineLayout; }
	private:
		void InvalidateRayTracingPipeline();
	private:
		std::string m_DebugName;

		VkPipeline m_RayTracingPipeline = nullptr;
		VkPipelineLayout m_PipelineLayout = nullptr;
		VkShaderModule m_RayGenShaderModule = nullptr;
		std::vector<VkShaderModule> m_RayClosestHitShaderModules, m_RayAnyHitShaderModules, m_RayIntersectionShaderModules, m_RayMissShaderModules;

		std::shared_ptr<ShaderBindingTable> m_ShaderBindingTable;
		std::vector<std::shared_ptr<VulkanDescriptorSetLayout>> m_DescriptorSetLayout;
	};

}
