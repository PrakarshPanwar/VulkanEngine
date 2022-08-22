#include "vulkanpch.h"
#include "VulkanDescriptor.h"

#include "Assert.h"
#include "Log.h"

namespace VulkanCore {

	DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(VulkanDevice& device)
		: m_VulkanDevice(device)
	{
	}

	DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count)
	{
		VK_CORE_ASSERT(m_Bindings.count(binding) == 0, "Binding already in Use!");
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count;
		layoutBinding.stageFlags = stageFlags;

		m_Bindings[binding] = layoutBinding;
		return *this;
	}

	std::unique_ptr<VulkanDescriptorSetLayout> DescriptorSetLayoutBuilder::Build() const
	{
		return std::make_unique<VulkanDescriptorSetLayout>(m_VulkanDevice, m_Bindings);
	}

	DescriptorPoolBuilder::DescriptorPoolBuilder(VulkanDevice& device)
		: m_VulkanDevice(device)
	{
	}

	DescriptorPoolBuilder& DescriptorPoolBuilder::AddPoolSize(VkDescriptorType descriptorType, uint32_t count)
	{
		m_PoolSizes.push_back({ descriptorType, count });
		return *this;
	}

	DescriptorPoolBuilder& DescriptorPoolBuilder::SetPoolFlags(VkDescriptorPoolCreateFlags flags)
	{
		m_PoolFlags = flags;
		return *this;
	}

	DescriptorPoolBuilder& DescriptorPoolBuilder::SetMaxSets(uint32_t count)
	{
		m_MaxSets = count;
		return *this;
	}

	std::unique_ptr<VulkanDescriptorPool> DescriptorPoolBuilder::Build() const
	{
		return std::make_unique<VulkanDescriptorPool>(m_VulkanDevice, m_MaxSets, m_PoolFlags, m_PoolSizes);
	}

	VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
		: m_VulkanDevice(device), m_Bindings(bindings)
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};

		for (auto kv : m_Bindings)
			setLayoutBindings.push_back(kv.second);

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_VulkanDevice.GetVulkanDevice(), &descriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout), "Failed to Create Descriptor Set Layout!");
	}

	VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(m_VulkanDevice.GetVulkanDevice(), m_DescriptorSetLayout, nullptr);
	}

	VulkanDescriptorPool::VulkanDescriptorPool(VulkanDevice& device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes)
		: m_VulkanDevice(device)
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		VK_CHECK_RESULT(vkCreateDescriptorPool(m_VulkanDevice.GetVulkanDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool), "Failed to Create Descriptor Pool!");
	}

	bool VulkanDescriptorPool::AllocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;

		// TODO: Might want to create a "DescriptorPoolManager" class that handles this case, and builds
		// a new pool whenever an old pool fills up. But this is beyond our current scope
		if (vkAllocateDescriptorSets(m_VulkanDevice.GetVulkanDevice(), &allocInfo, &descriptor) != VK_SUCCESS)
			return false;

		return true;
	}

	void VulkanDescriptorPool::FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const
	{
		vkFreeDescriptorSets(m_VulkanDevice.GetVulkanDevice(), m_DescriptorPool, static_cast<uint32_t>(descriptors.size()), descriptors.data());
	}

	void VulkanDescriptorPool::ResetPool()
	{
		vkResetDescriptorPool(m_VulkanDevice.GetVulkanDevice(), m_DescriptorPool, 0);
	}

	VulkanDescriptorPool::~VulkanDescriptorPool()
	{
		vkDestroyDescriptorPool(m_VulkanDevice.GetVulkanDevice(), m_DescriptorPool, nullptr);
	}

	VulkanDescriptorWriter::VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool)
		: m_SetLayout(setLayout), m_Pool(pool)
	{
	}

	VulkanDescriptorWriter& VulkanDescriptorWriter::WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
	{
		VK_CORE_ASSERT(m_SetLayout.m_Bindings.count(binding) == 1, "Layout does not contain specified Binding!");
		auto& bindingDescription = m_SetLayout.m_Bindings[binding];

		VK_CORE_ASSERT(bindingDescription.descriptorCount == 1, "Binding single Descriptor Info, but binding expects multiple");
	
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pBufferInfo = bufferInfo;
		write.descriptorCount = 1;

		m_Writes.push_back(write);
		return *this;
	}

	VulkanDescriptorWriter& VulkanDescriptorWriter::WriteImage(uint32_t binding, VkDescriptorImageInfo* imageInfo)
	{
		VK_CORE_ASSERT(m_SetLayout.m_Bindings.count(binding) == 1, "Layout does not contain specified Binding!");
		auto& bindingDescription = m_SetLayout.m_Bindings[binding];

		VK_CORE_ASSERT(bindingDescription.descriptorCount == 1, "Binding single Descriptor Info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pImageInfo = imageInfo;
		write.descriptorCount = 1;

		m_Writes.push_back(write);
		return *this;
	}

	VulkanDescriptorWriter& VulkanDescriptorWriter::WriteImage(uint32_t binding, const std::vector<VkDescriptorImageInfo>& imagesInfo)
	{
		VK_CORE_ASSERT(m_SetLayout.m_Bindings.count(binding) == 1, "Layout does not contain specified Binding!");
		auto& bindingDescription = m_SetLayout.m_Bindings[binding];

		//VK_CORE_ASSERT(bindingDescription.descriptorCount == 1, "Binding single Descriptor Info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pImageInfo = imagesInfo.data();
		write.descriptorCount = (uint32_t)imagesInfo.size();
		write.dstArrayElement = 0;

		m_Writes.push_back(write);
		return *this;
	}

	bool VulkanDescriptorWriter::Build(VkDescriptorSet& set)
	{
		bool success = m_Pool.AllocateDescriptor(m_SetLayout.GetDescriptorSetLayout(), set);

		if (!success)
			return false;

		Overwrite(set);
		return true;
	}

	void VulkanDescriptorWriter::Overwrite(VkDescriptorSet& set)
	{
		for (auto& write : m_Writes)
			write.dstSet = set;

		vkUpdateDescriptorSets(m_Pool.m_VulkanDevice.GetVulkanDevice(), (uint32_t)m_Writes.size(), m_Writes.data(), 0, nullptr);
	}

}