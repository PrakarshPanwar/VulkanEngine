#pragma once
#include "VulkanDevice.h"

namespace VulkanCore {

	class VulkanDescriptorSetLayout;

	class DescriptorSetLayoutBuilder
	{
	public:
		DescriptorSetLayoutBuilder(VulkanDevice& device);

		DescriptorSetLayoutBuilder& AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
		std::unique_ptr<VulkanDescriptorSetLayout> Build() const;
	private:
		VulkanDevice& m_VulkanDevice;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;
	};

	class VulkanDescriptorPool;

	class DescriptorPoolBuilder
	{
	public:
		DescriptorPoolBuilder(VulkanDevice& device);

		DescriptorPoolBuilder& AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
		DescriptorPoolBuilder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
		DescriptorPoolBuilder& SetMaxSets(uint32_t count);
		std::unique_ptr<VulkanDescriptorPool> Build() const;
	private:
		VulkanDevice& m_VulkanDevice;
		std::vector<VkDescriptorPoolSize> m_PoolSizes;
		uint32_t m_MaxSets = 1000;
		VkDescriptorPoolCreateFlags m_PoolFlags = 0;
	};

	class VulkanDescriptorSetLayout
	{
	public:
		VulkanDescriptorSetLayout(VulkanDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
		~VulkanDescriptorSetLayout();
		VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
		VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

		VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
	private:
		VulkanDevice& m_VulkanDevice;
		VkDescriptorSetLayout m_DescriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;

		friend class VulkanDescriptorWriter;
	};

	class VulkanDescriptorPool
	{
	public:
		VulkanDescriptorPool(VulkanDevice& device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes);
		~VulkanDescriptorPool();
		VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
		VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

		bool AllocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;
		void FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
		void ResetPool();

		inline VkDescriptorPool GetDescriptorPool() { return m_DescriptorPool; }
	private:
		VulkanDevice& m_VulkanDevice;
		VkDescriptorPool m_DescriptorPool;

		friend class VulkanDescriptorWriter;
	};

	class VulkanDescriptorWriter
	{
	public:
		VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool);

		VulkanDescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		VulkanDescriptorWriter& WriteImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);
		VulkanDescriptorWriter& WriteImage(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imagesInfo);

		bool Build(VkDescriptorSet& set);
		void Overwrite(VkDescriptorSet& set);
	private:
		VulkanDescriptorSetLayout& m_SetLayout;
		VulkanDescriptorPool& m_Pool;
		std::vector<VkWriteDescriptorSet> m_Writes;
	};

}