#include "vulkanpch.h"
#include "VulkanDescriptor.h"

#include "VulkanCore/Core/Core.h"

namespace VulkanCore {

	DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder()
	{
	}

	DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::AddBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count)
	{
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count;
		layoutBinding.stageFlags = stageFlags;

		m_Bindings[binding] = layoutBinding;
		return *this;
	}

	std::shared_ptr<VulkanDescriptorSetLayout> DescriptorSetLayoutBuilder::Build() const
	{
		return std::make_shared<VulkanDescriptorSetLayout>(m_Bindings);
	}

	DescriptorPoolBuilder::DescriptorPoolBuilder()
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

	std::shared_ptr<VulkanDescriptorPool> DescriptorPoolBuilder::Build() const
	{
		return std::make_shared<VulkanDescriptorPool>(m_MaxSets, m_PoolFlags, m_PoolSizes);
	}

	VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
		: m_Bindings(bindings)
	{
		auto device = VulkanContext::GetCurrentDevice();

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};

		for (const auto& kv : m_Bindings)
			setLayoutBindings.push_back(kv.second);

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->GetVulkanDevice(), &descriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout), "Failed to Create Descriptor Set Layout!");
	}

	VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), m_DescriptorSetLayout, nullptr);
	}

	VulkanDescriptorPool::VulkanDescriptorPool(uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize>& poolSizes)
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		VK_CHECK_RESULT(vkCreateDescriptorPool(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), &descriptorPoolInfo, nullptr, &m_DescriptorPool), "Failed to Create Descriptor Pool!");
	}

	void VulkanDescriptorPool::AllocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;

		// TODO: Might want to create a "DescriptorPoolManager" class that handles this case, and builds
		// a new pool whenever an old pool fills up. But this is beyond our current scope
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->GetVulkanDevice(), &allocInfo, &descriptor), "Error occured in allocating Descriptor!");
	}

	void VulkanDescriptorPool::AllocateDescriptorSet(const std::vector<VkDescriptorSetLayout>& descriptorSetsLayout, const std::vector<VkDescriptorSet>& descriptorSets)
	{
		auto device = VulkanContext::GetCurrentDevice();

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.pSetLayouts = descriptorSetsLayout.data();
		allocInfo.descriptorSetCount = (uint32_t)descriptorSetsLayout.size();

		// TODO: Might want to create a "DescriptorPoolManager" class that handles this case, and builds
		// a new pool whenever an old pool fills up. But this is beyond our current scope
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->GetVulkanDevice(), &allocInfo, (VkDescriptorSet*)descriptorSets.data()), "Error occured in allocating Descriptors!");
	}

	void VulkanDescriptorPool::FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const
	{
		auto device = VulkanContext::GetCurrentDevice();
		vkFreeDescriptorSets(device->GetVulkanDevice(), m_DescriptorPool, (uint32_t)descriptors.size(), descriptors.data());
	}

	void VulkanDescriptorPool::ResetPool()
	{
		vkResetDescriptorPool(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), m_DescriptorPool, 0);
	}

	VulkanDescriptorPool::~VulkanDescriptorPool()
	{
		vkDestroyDescriptorPool(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), m_DescriptorPool, nullptr);
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

	VulkanDescriptorWriter& VulkanDescriptorWriter::WriteImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo)
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

	void VulkanDescriptorWriter::Build(VkDescriptorSet& set)
	{
		m_Pool.AllocateDescriptorSet(m_SetLayout.GetVulkanDescriptorSetLayout(), set);
		Overwrite(set);
	}

	void VulkanDescriptorWriter::Overwrite(VkDescriptorSet& set)
	{
		for (auto& write : m_Writes)
			write.dstSet = set;

		vkUpdateDescriptorSets(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)m_Writes.size(), m_Writes.data(), 0, nullptr);
	}

}
