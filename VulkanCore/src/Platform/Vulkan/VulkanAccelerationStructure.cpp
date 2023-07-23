#include "vulkanpch.h"
#include "VulkanAccelerationStructure.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanAllocator.h"

namespace VulkanCore {

	namespace Utils {

		static VkDeviceOrHostAddressConstKHR GetBufferDeviceAddress(VkBuffer buffer)
		{
			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
			bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			bufferDeviceAddressInfo.buffer = buffer;
			return (VkDeviceOrHostAddressConstKHR)vkGetBufferDeviceAddressKHR(vulkanDevice, &bufferDeviceAddressInfo);
		}

	}

	VulkanAccelerationStructure::VulkanAccelerationStructure()
	{
	}

	VulkanAccelerationStructure::~VulkanAccelerationStructure()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("AccelerationStructure");

		// Bottom Level AS
		allocator.DestroyBuffer(m_BottomLevelAccelerationStructureInfo.Buffer, m_BottomLevelAccelerationStructureInfo.MemoryAlloc);
		vkDestroyAccelerationStructureKHR(device->GetVulkanDevice(), m_BottomLevelAccelerationStructureInfo.Handle, nullptr);

		// Top Level AS
		allocator.DestroyBuffer(m_TopLevelAccelerationStructureInfo.Buffer, m_TopLevelAccelerationStructureInfo.MemoryAlloc);
		vkDestroyAccelerationStructureKHR(device->GetVulkanDevice(), m_TopLevelAccelerationStructureInfo.Handle, nullptr);
	}

	void VulkanAccelerationStructure::BuildTopLevelAccelerationStructure()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("TopLevelAccelerationStructure");

		VkTransformMatrixKHR transformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f };

		VkAccelerationStructureInstanceKHR instanceData{};
		instanceData.transform = transformMatrix;
		instanceData.instanceCustomIndex = 0;
		instanceData.mask = 0xFF;
		instanceData.instanceShaderBindingTableRecordOffset = 0;
		instanceData.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instanceData.accelerationStructureReference = m_BottomLevelAccelerationStructureInfo.DeviceAddress;

		// Buffer for Instance Data
		VkBuffer instanceBuffer;
		VmaAllocation instanceBufferAlloc;

		VkBufferCreateInfo instanceBufferCreateInfo{};
		instanceBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		instanceBufferCreateInfo.size = sizeof(VkAccelerationStructureInstanceKHR);
		instanceBufferCreateInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		instanceBufferAlloc = allocator.AllocateBuffer(instanceBufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, instanceBuffer);

		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress = Utils::GetBufferDeviceAddress(instanceBuffer);

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

		// Get Sizes Info
		VkAccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{};
		asBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		asBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		asBuildGeometryInfo.geometryCount = 1;
		asBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		uint32_t primitiveCount = 1;

		VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
		buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		vkGetAccelerationStructureBuildSizesKHR(device->GetVulkanDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&asBuildGeometryInfo,
			&primitiveCount,
			&buildSizesInfo);

		// Create Acceleration Structure
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buildSizesInfo.accelerationStructureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		VkBuffer buffer;
		VmaAllocation memAlloc;
		memAlloc = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, buffer);

		m_TopLevelAccelerationStructureInfo.Buffer = buffer;
		m_TopLevelAccelerationStructureInfo.MemoryAlloc = memAlloc;

		// Acceleration Structure
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = buffer;
		accelerationStructureCreateInfo.size = buildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

		vkCreateAccelerationStructureKHR(device->GetVulkanDevice(),
			&accelerationStructureCreateInfo,
			nullptr,
			&m_TopLevelAccelerationStructureInfo.Handle);

		// AS Device Address
		VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo{};
		deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		deviceAddressInfo.accelerationStructure = m_TopLevelAccelerationStructureInfo.Handle;
		m_TopLevelAccelerationStructureInfo.DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device->GetVulkanDevice(), &deviceAddressInfo);

		// Create a Small Scratch Buffer used during build of the Bottom Level Acceleration Structure
		VkBuffer scratchBuffer;
		VmaAllocation scratchBufferAlloc;

		VkBufferCreateInfo scratchBufferCreateInfo{};
		scratchBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		scratchBufferCreateInfo.size = buildSizesInfo.buildScratchSize;
		scratchBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		scratchBufferAlloc = allocator.AllocateBuffer(scratchBufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, scratchBuffer);

		// Scratch Buffer Device Address
		VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
		bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddressInfo.buffer = scratchBuffer;
		VkDeviceAddress scratchBufferDeviceAddress = vkGetBufferDeviceAddressKHR(device->GetVulkanDevice(), &bufferDeviceAddressInfo);

		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
		buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildGeometryInfo.dstAccelerationStructure = m_TopLevelAccelerationStructureInfo.Handle;
		buildGeometryInfo.geometryCount = 1;
		buildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		buildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
		buildRangeInfo.primitiveCount = 1;
		buildRangeInfo.primitiveOffset = 0;
		buildRangeInfo.firstVertex = 0;
		buildRangeInfo.transformOffset = 0;

		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos = { &buildRangeInfo };

		// Build the Acceleration Structure on the device via a one-time command buffer submission
		// Some implementations may support Acceleration Structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		VkCommandBuffer buildCmd = device->GetCommandBuffer();

		vkCmdBuildAccelerationStructuresKHR(buildCmd,
			1,
			&buildGeometryInfo,
			pBuildRangeInfos.data());

		device->FlushCommandBuffer(buildCmd);

		allocator.DestroyBuffer(scratchBuffer, scratchBufferAlloc);
		allocator.DestroyBuffer(instanceBuffer, instanceBufferAlloc);

		m_DescriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		m_DescriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		m_DescriptorAccelerationStructureInfo.pAccelerationStructures = &m_TopLevelAccelerationStructureInfo.Handle;
	}

	void VulkanAccelerationStructure::BuildBottomLevelAccelerationStructure()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("BottomLevelAccelerationStructure");

		// Get Sizes Info
		VkAccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{};
		asBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		asBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		asBuildGeometryInfo.geometryCount = (uint32_t)m_AccelerationStructuresGeometries.size();
		asBuildGeometryInfo.pGeometries = m_AccelerationStructuresGeometries.data();

		VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
		buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		vkGetAccelerationStructureBuildSizesKHR(device->GetVulkanDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&asBuildGeometryInfo,
			m_GeometryCount.data(),
			&buildSizesInfo);

		// Create Acceleration Structure
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buildSizesInfo.accelerationStructureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		VkBuffer buffer;
		VmaAllocation memAlloc;
		memAlloc = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, buffer);

		m_BottomLevelAccelerationStructureInfo.Buffer = buffer;
		m_BottomLevelAccelerationStructureInfo.MemoryAlloc = memAlloc;

		// Acceleration Structure
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = buffer;
		accelerationStructureCreateInfo.size = buildSizesInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

		vkCreateAccelerationStructureKHR(device->GetVulkanDevice(),
			&accelerationStructureCreateInfo,
			nullptr,
			&m_BottomLevelAccelerationStructureInfo.Handle);

		// AS Device Address(Will be used as reference in creating Top Level AS)
		VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo{};
		deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		deviceAddressInfo.accelerationStructure = m_BottomLevelAccelerationStructureInfo.Handle;
		m_BottomLevelAccelerationStructureInfo.DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device->GetVulkanDevice(), &deviceAddressInfo);

		// Create a Small Scratch Buffer used during build of the Bottom Level Acceleration Structure
		VkBuffer scratchBuffer;
		VmaAllocation scratchBufferAlloc;

		VkBufferCreateInfo scratchBufferCreateInfo{};
		scratchBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		scratchBufferCreateInfo.size = buildSizesInfo.buildScratchSize;
		scratchBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		scratchBufferAlloc = allocator.AllocateBuffer(scratchBufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, scratchBuffer);

		// Scratch Buffer Device Address
		VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
		bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddressInfo.buffer = scratchBuffer;
		VkDeviceAddress scratchBufferDeviceAddress = vkGetBufferDeviceAddressKHR(device->GetVulkanDevice(), &bufferDeviceAddressInfo);

		// Build Acceleration Structure
		VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
		buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		buildGeometryInfo.geometryCount = (uint32_t)m_AccelerationStructuresGeometries.size();
		buildGeometryInfo.pGeometries = m_AccelerationStructuresGeometries.data();
		buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildGeometryInfo.dstAccelerationStructure = m_BottomLevelAccelerationStructureInfo.Handle;
		buildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos;
		for (uint32_t i = 0; i < m_AccelerationStructuresBuildRanges.size(); ++i)
			pBuildRangeInfos.push_back(&m_AccelerationStructuresBuildRanges[i]);

		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		VkCommandBuffer buildCmd = device->GetCommandBuffer();

		vkCmdBuildAccelerationStructuresKHR(buildCmd,
			1,
			&buildGeometryInfo,
			pBuildRangeInfos.data());

		device->FlushCommandBuffer(buildCmd);

		allocator.DestroyBuffer(scratchBuffer, scratchBufferAlloc);

		// Clear Geometry Data
		m_AccelerationStructuresGeometries.clear();
		m_AccelerationStructuresBuildRanges.clear();
	}

	void VulkanAccelerationStructure::SubmitMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<VertexBuffer> transformBuffer, uint32_t transformOffset)
	{
		auto meshSource = mesh->GetMeshSource();
		auto& submeshData = meshSource->GetSubmeshes();

		auto vulkanMeshVB = std::static_pointer_cast<VulkanVertexBuffer>(meshSource->GetVertexBuffer());
		auto vulkanMeshIB = std::static_pointer_cast<VulkanIndexBuffer>(meshSource->GetIndexBuffer());
		auto vulkanTransformBuffer = std::static_pointer_cast<VulkanVertexBuffer>(transformBuffer);

		VkDeviceOrHostAddressConstKHR vertexBufferAddress = Utils::GetBufferDeviceAddress(vulkanMeshVB->GetVulkanBuffer());
		VkDeviceOrHostAddressConstKHR indexBufferAddress = Utils::GetBufferDeviceAddress(vulkanMeshIB->GetVulkanBuffer());
		VkDeviceOrHostAddressConstKHR transformBufferAddress = Utils::GetBufferDeviceAddress(vulkanTransformBuffer->GetVulkanBuffer());

		uint32_t index = 0;
		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			VkAccelerationStructureGeometryTrianglesDataKHR trianglesData{};
			trianglesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			trianglesData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
			trianglesData.vertexData = vertexBufferAddress;
			trianglesData.maxVertex = meshSource->GetVertexCount();
			trianglesData.vertexStride = sizeof(Vertex); // We may have to change this as our current stride is struct Vertex
			trianglesData.indexType = VK_INDEX_TYPE_UINT32;
			trianglesData.indexData = indexBufferAddress;
			trianglesData.transformData = transformBufferAddress;

			VkAccelerationStructureGeometryKHR geometryData{};
			geometryData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			geometryData.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
			geometryData.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			geometryData.geometry.triangles = trianglesData;

			const Submesh& submesh = submeshData[index++];

			VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
			buildRangeInfo.firstVertex = submesh.BaseVertex;
			buildRangeInfo.primitiveCount = submesh.IndexCount / 3;
			buildRangeInfo.primitiveOffset = submesh.BaseIndex;
			buildRangeInfo.transformOffset = transformOffset * sizeof(VkTransformMatrixKHR); // TODO: Find a way to set transform offset index

			m_AccelerationStructuresGeometries.push_back(geometryData);
			m_AccelerationStructuresBuildRanges.push_back(buildRangeInfo);
			m_GeometryCount.push_back(1);
		}
	}

}
