#pragma once
#include "VulkanContext.h"

namespace VulkanCore {

	class VulkanDescriptorSetLayout;

	class DescriptorSetLayoutBuilder
	{
	public:
		DescriptorSetLayoutBuilder();

		DescriptorSetLayoutBuilder& AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
		std::shared_ptr<VulkanDescriptorSetLayout> Build() const;
	private:
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;

		friend class VulkanMaterial;
	};

	class VulkanDescriptorPool;

	class DescriptorPoolBuilder
	{
	public:
		DescriptorPoolBuilder();

		DescriptorPoolBuilder& AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
		DescriptorPoolBuilder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
		DescriptorPoolBuilder& SetMaxSets(uint32_t count);
		std::shared_ptr<VulkanDescriptorPool> Build() const;
	private:
		std::vector<VkDescriptorPoolSize> m_PoolSizes;
		uint32_t m_MaxSets = 1000;
		VkDescriptorPoolCreateFlags m_PoolFlags = 0;

		friend class VulkanMaterial;
	};

	class VulkanDescriptorSetLayout
	{
	public:
		VulkanDescriptorSetLayout(std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
		~VulkanDescriptorSetLayout();
		VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
		VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

		VkDescriptorSetLayout GetVulkanDescriptorSetLayout() const { return m_DescriptorSetLayout; }
		VkDescriptorType GetVulkanDescriptorType(uint32_t binding) const { return m_Bindings.at(binding).descriptorType; }
	private:
		VkDescriptorSetLayout m_DescriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;

		friend class VulkanDescriptorWriter;
		friend class VulkanMaterial;
	};

	class VulkanDescriptorPool
	{
	public:
		VulkanDescriptorPool(uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes);
		~VulkanDescriptorPool();
		VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
		VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

		bool AllocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;
		bool AllocateDescriptorSet(const std::vector<VkDescriptorSetLayout>& descriptorSetsLayout, const std::vector<VkDescriptorSet>& descriptorSets);
		void FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
		void ResetPool();

		inline VkDescriptorPool GetDescriptorPool() { return m_DescriptorPool; }
	private:
		VkDescriptorPool m_DescriptorPool;

		friend class VulkanDescriptorWriter;
	};

	class VulkanDescriptorWriter
	{
	public:
		VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool);

		VulkanDescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		VulkanDescriptorWriter& WriteImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo);
		VulkanDescriptorWriter& WriteImage(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imagesInfo);

		bool Build(VkDescriptorSet& set);
		void Overwrite(VkDescriptorSet& set);
	private:
		VulkanDescriptorSetLayout& m_SetLayout;
		VulkanDescriptorPool& m_Pool;
		std::vector<VkWriteDescriptorSet> m_Writes;

		friend class VulkanMaterial;
	};

}