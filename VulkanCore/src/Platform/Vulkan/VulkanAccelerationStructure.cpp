#include "vulkanpch.h"
#include "VulkanAccelerationStructure.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanAllocator.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Core/Components.h"
#include "VulkanCore/Asset/AssetManager.h"

namespace VulkanCore {

	namespace Utils {

		static VkDeviceOrHostAddressConstKHR GetBufferDeviceAddress(VkBuffer buffer)
		{
			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
			bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			bufferDeviceAddressInfo.buffer = buffer;

			VkDeviceOrHostAddressConstKHR deviceConstAddress{};
			deviceConstAddress.deviceAddress = vkGetBufferDeviceAddressKHR(vulkanDevice, &bufferDeviceAddressInfo);
			return deviceConstAddress;
		}

		static VkTransformMatrixKHR VulkanTransformFromTransformData(const TransformData& transformData)
		{
			VkTransformMatrixKHR vulkanTransform{};

			vulkanTransform = {
				transformData.MRow[0].x, transformData.MRow[0].y, transformData.MRow[0].z, transformData.MRow[0].w,
				transformData.MRow[1].x, transformData.MRow[1].y, transformData.MRow[1].z, transformData.MRow[1].w,
				transformData.MRow[2].x, transformData.MRow[2].y, transformData.MRow[2].z, transformData.MRow[2].w
			};

			return vulkanTransform;
		}

	}

	VulkanAccelerationStructure::VulkanAccelerationStructure()
	{
		m_DescriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		m_DescriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		m_DescriptorAccelerationStructureInfo.pAccelerationStructures = nullptr;
	}

	VulkanAccelerationStructure::~VulkanAccelerationStructure()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("AccelerationStructure");

		// Bottom Level AS
		for (auto& [mk, blasInput] : m_BLASInputData)
		{
			allocator.DestroyBuffer(blasInput.BLASInfo.Buffer, blasInput.BLASInfo.MemoryAlloc);
			vkDestroyAccelerationStructureKHR(device->GetVulkanDevice(), blasInput.BLASInfo.Handle, nullptr);
		}

		// Top Level AS
		allocator.DestroyBuffer(m_TLASInfo.Buffer, m_TLASInfo.MemoryAlloc);
		vkDestroyAccelerationStructureKHR(device->GetVulkanDevice(), m_TLASInfo.Handle, nullptr);

		// Instance Buffer
		allocator.DestroyBuffer(m_InstanceBuffer, m_InstanceBufferAlloc);
	}

	void VulkanAccelerationStructure::BuildTopLevelAccelerationStructure()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("TopLevelAccelerationStructure");

		std::unique_ptr<Timer> timer = std::make_unique<Timer>("Top Level Acceleration Structure");

		// Batch all instance data from BLAS Input
		std::vector<VkAccelerationStructureInstanceKHR> tlasInstanceData{};
		for (auto& [mk, blasInput] : m_BLASInputData)
			tlasInstanceData.insert(tlasInstanceData.end(), blasInput.InstanceData.begin(), blasInput.InstanceData.end());

		// Buffer for Instance Data
		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAlloc;

		uint32_t instanceBufferSize = tlasInstanceData.size() * sizeof(VkAccelerationStructureInstanceKHR);

		VkBufferCreateInfo stagingBufferCreateInfo{};
		stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		stagingBufferCreateInfo.size = instanceBufferSize;
		stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		stagingBufferAlloc = allocator.AllocateBuffer(stagingBufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, stagingBuffer);

		// Device Instance Buffer
		VkBufferCreateInfo instanceBufferCreateInfo{};
		instanceBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		instanceBufferCreateInfo.size = instanceBufferSize;
		instanceBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

		m_InstanceBufferAlloc = allocator.AllocateBuffer(instanceBufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, m_InstanceBuffer);

		// Copy Instance Data to Staging(Host) Buffer
		uint8_t* stagingDstData = allocator.MapMemory<uint8_t>(stagingBufferAlloc);
		memcpy(stagingDstData, tlasInstanceData.data(), instanceBufferSize);
		allocator.UnmapMemory(stagingBufferAlloc);

		// Copy Instance Data to Device Buffer
		VkCommandBuffer copyCmd = device->GetCommandBuffer();

		VkBufferCopy copyRegion{};
		copyRegion.size = instanceBufferSize;

		vkCmdCopyBuffer(copyCmd,
			stagingBuffer,
			m_InstanceBuffer,
			1,
			&copyRegion
		);

		device->FlushCommandBuffer(copyCmd);

		// Destroy Staging Buffer
		allocator.DestroyBuffer(stagingBuffer, stagingBufferAlloc);

		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress = Utils::GetBufferDeviceAddress(m_InstanceBuffer);

		VkAccelerationStructureGeometryInstancesDataKHR instances{};
		instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		instances.arrayOfPointers = VK_FALSE;
		instances.data = instanceDataDeviceAddress;

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometry.instances = instances;

		// Get Sizes Info
		VkAccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{};
		asBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		asBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		asBuildGeometryInfo.geometryCount = 1;
		asBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
		buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		uint32_t instanceCount = (uint32_t)tlasInstanceData.size();
		vkGetAccelerationStructureBuildSizesKHR(device->GetVulkanDevice(),
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&asBuildGeometryInfo,
			&instanceCount,
			&buildSizesInfo);

		// Create Acceleration Structure
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buildSizesInfo.accelerationStructureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		VkBuffer buffer;
		VmaAllocation memAlloc;
		memAlloc = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, buffer);

		m_TLASInfo.Buffer = buffer;
		m_TLASInfo.MemoryAlloc = memAlloc;

		// Acceleration Structure
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureCreateInfo.buffer = buffer;
		accelerationStructureCreateInfo.size = buildSizesInfo.accelerationStructureSize;

		vkCreateAccelerationStructureKHR(device->GetVulkanDevice(),
			&accelerationStructureCreateInfo,
			nullptr,
			&m_TLASInfo.Handle);

		// AS Device Address
		VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo{};
		deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		deviceAddressInfo.accelerationStructure = m_TLASInfo.Handle;
		m_TLASInfo.DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device->GetVulkanDevice(), &deviceAddressInfo);

		// Create a Small Scratch Buffer used during build of the Top Level Acceleration Structure
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
		buildGeometryInfo.dstAccelerationStructure = m_TLASInfo.Handle;
		buildGeometryInfo.geometryCount = 1;
		buildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		buildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
		buildRangeInfo.firstVertex = 0;
		buildRangeInfo.primitiveCount = (uint32_t)tlasInstanceData.size();
		buildRangeInfo.primitiveOffset = 0;
		buildRangeInfo.transformOffset = 0;

		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos = { &buildRangeInfo };

		// Build the Acceleration Structure on the device via a one-time command buffer submission
		// Some implementations may support Acceleration Structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		VkCommandBuffer buildCmd = device->GetCommandBuffer();

		// Make sure the Copy of the Instance Buffer are copied before triggering the Acceleration Structure Build
 		VkMemoryBarrier instanceBufferBarrier{};
 		instanceBufferBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
 		instanceBufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
 		instanceBufferBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
 
 		vkCmdPipelineBarrier(buildCmd,
 			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
 			0, 1, &instanceBufferBarrier, 0, nullptr, 0, nullptr);

		vkCmdBuildAccelerationStructuresKHR(buildCmd,
			1,
			&buildGeometryInfo,
			pBuildRangeInfos.data());

		device->FlushCommandBuffer(buildCmd);

		allocator.DestroyBuffer(scratchBuffer, scratchBufferAlloc);

		VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(), VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, "Top Level Acceleration Structure", m_TLASInfo.Handle);

		m_DescriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		m_DescriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		m_DescriptorAccelerationStructureInfo.pAccelerationStructures = &m_TLASInfo.Handle;
	}

	void VulkanAccelerationStructure::BuildBottomLevelAccelerationStructures()
	{
		auto device = VulkanContext::GetCurrentDevice();
		VulkanAllocator allocator("BottomLevelAccelerationStructure");

		std::unique_ptr<Timer> timer = std::make_unique<Timer>("Bottom Level Acceleration Structure");

#define USE_BLAS_MAP 1
#if USE_BLAS_MAP
		uint32_t inputSize = (uint32_t)m_BLASInputData.size();
		for (auto& [mk, blasInput] : m_BLASInputData)
		{
			// Get Sizes Info
			VkAccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{};
			asBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			asBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			asBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			asBuildGeometryInfo.geometryCount = 1;
			asBuildGeometryInfo.pGeometries = &blasInput.GeometryData;

			VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
			buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

			vkGetAccelerationStructureBuildSizesKHR(device->GetVulkanDevice(),
				VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				&asBuildGeometryInfo,
				&blasInput.BuildRangeInfo.primitiveCount,
				&buildSizesInfo);

			// Create Acceleration Structure
			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = buildSizesInfo.accelerationStructureSize;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

			VkBuffer buffer;
			VmaAllocation memAlloc;
			memAlloc = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, buffer);

			VulkanAccelerationStructureInfo& blasInfo = blasInput.BLASInfo;
			blasInfo.Buffer = buffer;
			blasInfo.MemoryAlloc = memAlloc;

			// Acceleration Structure
			VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
			accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			accelerationStructureCreateInfo.buffer = buffer;
			accelerationStructureCreateInfo.size = buildSizesInfo.accelerationStructureSize;

			vkCreateAccelerationStructureKHR(device->GetVulkanDevice(),
				&accelerationStructureCreateInfo,
				nullptr,
				&blasInfo.Handle);

			// AS Device Address(Will be used as reference in creating Top Level AS)
			VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo{};
			deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
			deviceAddressInfo.accelerationStructure = blasInfo.Handle;
			blasInfo.DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device->GetVulkanDevice(), &deviceAddressInfo);

			// Update Instance for TLAS Build
			for (auto& instance : blasInput.InstanceData)
				instance.accelerationStructureReference = blasInfo.DeviceAddress;

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
			buildGeometryInfo.geometryCount = 1;
			buildGeometryInfo.pGeometries = &blasInput.GeometryData;
			buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			buildGeometryInfo.dstAccelerationStructure = blasInfo.Handle;
			buildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

			std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfo = { &blasInput.BuildRangeInfo };

			// Build the acceleration structure on the device via a one-time command buffer submission
			// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
			VkCommandBuffer buildCmd = device->GetCommandBuffer();

			vkCmdBuildAccelerationStructuresKHR(buildCmd,
				1,
				&buildGeometryInfo,
				pBuildRangeInfo.data());

			device->FlushCommandBuffer(buildCmd);

			allocator.DestroyBuffer(scratchBuffer, scratchBufferAlloc);
		}
#else
		// BLAS Info Resize
		m_BLASInfoData.resize(m_BLASGeometryData.size());

		for (uint32_t i = 0; i < m_BLASGeometryData.size(); ++i)
		{
			// Get Sizes Info
			VkAccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo{};
			asBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			asBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			asBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			asBuildGeometryInfo.geometryCount = 1;
			asBuildGeometryInfo.pGeometries = &m_BLASGeometryData[i];

			VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
			buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

			vkGetAccelerationStructureBuildSizesKHR(device->GetVulkanDevice(),
				VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				&asBuildGeometryInfo,
				&m_BLASBuildRanges[i].primitiveCount,
				&buildSizesInfo);

			// Create Acceleration Structure
			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = buildSizesInfo.accelerationStructureSize;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

			VkBuffer buffer;
			VmaAllocation memAlloc;
			memAlloc = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, buffer);

			VulkanAccelerationStructureInfo& blasInfo = m_BLASInfoData[i];
			blasInfo.Buffer = buffer;
			blasInfo.MemoryAlloc = memAlloc;

			// Acceleration Structure
			VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
			accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			accelerationStructureCreateInfo.buffer = buffer;
			accelerationStructureCreateInfo.size = buildSizesInfo.accelerationStructureSize;

			vkCreateAccelerationStructureKHR(device->GetVulkanDevice(),
				&accelerationStructureCreateInfo,
				nullptr,
				&blasInfo.Handle);

			// AS Device Address(Will be used as reference in creating Top Level AS)
			VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo{};
			deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
			deviceAddressInfo.accelerationStructure = blasInfo.Handle;
			blasInfo.DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device->GetVulkanDevice(), &deviceAddressInfo);

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
			buildGeometryInfo.geometryCount = (uint32_t)m_BLASGeometryData.size();
			buildGeometryInfo.pGeometries = m_BLASGeometryData.data();
			buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			buildGeometryInfo.dstAccelerationStructure = blasInfo.Handle;
			buildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

			std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfo = { &m_BLASBuildRanges[i] };

			// Build the acceleration structure on the device via a one-time command buffer submission
			// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
			VkCommandBuffer buildCmd = device->GetCommandBuffer();

			vkCmdBuildAccelerationStructuresKHR(buildCmd,
				1,
				&buildGeometryInfo,
				pBuildRangeInfo.data());

			device->FlushCommandBuffer(buildCmd);

			allocator.DestroyBuffer(scratchBuffer, scratchBufferAlloc);
		}

		// Clear Geometry Data
		m_BLASGeometryData.clear();
		m_BLASBuildRanges.clear();
#endif
	}

	void VulkanAccelerationStructure::SubmitMeshDrawData(const MeshKey& meshKey, const std::vector<TransformData>& transformData, uint32_t instanceCount)
	{
		auto mesh = AssetManager::GetAsset<Mesh>(meshKey.MeshHandle);
		auto meshSource = mesh->GetMeshSource();

		auto vulkanMeshVB = std::static_pointer_cast<VulkanVertexBuffer>(meshSource->GetVertexBuffer());
		auto vulkanMeshIB = std::static_pointer_cast<VulkanIndexBuffer>(meshSource->GetIndexBuffer());

		VkDeviceOrHostAddressConstKHR vertexBufferAddress = Utils::GetBufferDeviceAddress(vulkanMeshVB->GetVulkanBuffer());
		VkDeviceOrHostAddressConstKHR indexBufferAddress = Utils::GetBufferDeviceAddress(vulkanMeshIB->GetVulkanBuffer());

		VkAccelerationStructureGeometryTrianglesDataKHR trianglesData{};
		trianglesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		trianglesData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		trianglesData.vertexData = vertexBufferAddress;
		trianglesData.maxVertex = meshSource->GetVertexCount();
		trianglesData.vertexStride = sizeof(Vertex); // We may have to change this as our current stride is struct Vertex
		trianglesData.indexType = VK_INDEX_TYPE_UINT32;
		trianglesData.indexData = indexBufferAddress;
		trianglesData.transformData = { 0 };

		VkAccelerationStructureGeometryKHR geometryData{};
		geometryData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometryData.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		geometryData.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometryData.geometry.triangles = trianglesData;

		uint32_t submeshIndex = meshKey.SubmeshIndex;
		auto& submeshData = meshSource->GetSubmeshes();
		const Submesh& submesh = submeshData[submeshIndex];

		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
		buildRangeInfo.firstVertex = submesh.BaseVertex;
		buildRangeInfo.primitiveCount = submesh.IndexCount / 3;
		buildRangeInfo.primitiveOffset = submesh.BaseIndex;
		buildRangeInfo.transformOffset = 0;

		auto& blasInput = m_BLASInputData[meshKey];
		blasInput.GeometryData = geometryData;
		blasInput.BuildRangeInfo = buildRangeInfo;
		blasInput.InstanceCount = instanceCount;

		uint32_t index = 0;
		std::vector<VkAccelerationStructureInstanceKHR> instances{};
		instances.resize(instanceCount);

		for (uint32_t i = 0; i < instanceCount; ++i)
		{
			VkTransformMatrixKHR vulkanTransform = Utils::VulkanTransformFromTransformData(transformData[i]);

			// NOTE: BLAS Reference should be updated after BLAS Build(i.e. accelerationStructureReference)
			VkAccelerationStructureInstanceKHR instanceData{};
			instanceData.transform = vulkanTransform;
			instanceData.instanceCustomIndex = index++;
			instanceData.mask = 0xFF;
			instanceData.instanceShaderBindingTableRecordOffset = 0;
			instanceData.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			instanceData.accelerationStructureReference = blasInput.BLASInfo.DeviceAddress;

			instances[i] = instanceData;
		}

		blasInput.InstanceData = instances;
	}

}
