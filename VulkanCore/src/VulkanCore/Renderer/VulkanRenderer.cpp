#include "vulkanpch.h"
#include "VulkanRenderer.h"

#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Scene/SceneRenderer.h"
#include "Renderer.h"
#include "Platform/Vulkan/VulkanContext.h"
#include "Platform/Vulkan/VulkanDescriptor.h"
#include "Platform/Vulkan/VulkanMaterial.h"

#include <glm/gtx/integer.hpp>
#include "optick.h"

namespace VulkanCore {

	VulkanRenderer* VulkanRenderer::s_Instance;
	RendererStats VulkanRenderer::s_Data;

	VulkanRenderer::VulkanRenderer(std::shared_ptr<WindowsWindow> window)
		: m_Window(window)
	{
		s_Instance = this;
		Init();
	}

	VulkanRenderer::~VulkanRenderer()
	{
	}

	void VulkanRenderer::Init()
	{
		RecreateSwapChain();
		CreateCommandBuffers();
	}

	void VulkanRenderer::BeginFrame()
	{
		VK_CORE_PROFILE();

		VK_CORE_ASSERT(!IsFrameStarted, "Cannot call BeginFrame() while frame being already in progress!");

		Renderer::Submit([this]
		{
			auto result = m_SwapChain->AcquireNextImage(&m_CurrentImageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				RecreateSwapChain();
				return;
			}

			VK_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to Acquire Swap Chain!");
		});

		IsFrameStarted = true;

		m_CommandBuffer->Begin();
	}

	void VulkanRenderer::EndFrame()
	{
		VK_CORE_ASSERT(IsFrameStarted, "Cannot call EndFrame() while frame is not in progress!");
		m_CommandBuffer->End();
	}

	void VulkanRenderer::BeginScene()
	{
		auto commandBuffer = SceneRenderer::GetSceneRenderer()->GetCommandBuffer();
		commandBuffer->Begin();
	}

	void VulkanRenderer::EndScene()
	{
		auto commandBuffer = SceneRenderer::GetSceneRenderer()->GetCommandBuffer();
		commandBuffer->End();
	}

	void VulkanRenderer::BeginSwapChainRenderPass()
	{
		VK_CORE_ASSERT(IsFrameStarted, "Cannot call BeginSwapChainRenderPass() if frame is not in progress!");
		//VK_CORE_ASSERT(commandBuffer == GetCurrentCommandBuffer(), "Cannot begin Render Pass on Command Buffer from a different frame!");
	
		Renderer::Submit([this]
		{
			VkCommandBuffer commandBuffer = m_CommandBuffer->RT_GetActiveCommandBuffer();

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_SwapChain->GetRenderPass();
			renderPassInfo.framebuffer = m_SwapChain->GetFramebuffer(m_CurrentImageIndex);
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_SwapChain->GetSwapChainExtent();

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };

			renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(m_SwapChain->GetSwapChainExtent().width);
			viewport.height = static_cast<float>(m_SwapChain->GetSwapChainExtent().height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor{ { 0, 0 }, m_SwapChain->GetSwapChainExtent() };
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		});
	}

	void VulkanRenderer::EndSwapChainRenderPass()
	{
		VK_CORE_ASSERT(IsFrameStarted, "Cannot call EndSwapChainRenderPass() if frame is not in progress!");
		//VK_CORE_ASSERT(commandBuffer == GetCurrentCommandBuffer(), "Cannot end Render Pass on Command Buffer from a different frame!");

		Renderer::Submit([this]
		{
			VkCommandBuffer commandBuffer =	m_CommandBuffer->RT_GetActiveCommandBuffer();
			vkCmdEndRenderPass(commandBuffer);
		});
	}

	std::tuple<std::shared_ptr<VulkanTextureCube>, std::shared_ptr<VulkanTextureCube>> VulkanRenderer::CreateEnviromentMap(const std::string& filepath)
	{
		constexpr uint32_t cubemapSize = 1024;
		constexpr uint32_t irradianceMapSize = 32;

		std::shared_ptr<VulkanTexture> envEquirect = std::make_shared<VulkanTexture>(filepath);

		std::shared_ptr<VulkanTextureCube> envFiltered = std::make_shared<VulkanTextureCube>(cubemapSize, cubemapSize, ImageFormat::RGBA32F);
		std::shared_ptr<VulkanTextureCube> envUnfiltered = std::make_shared<VulkanTextureCube>(cubemapSize, cubemapSize, ImageFormat::RGBA32F);

		envFiltered->Invalidate();
		envUnfiltered->Invalidate();

		auto equirectangularConversionShader = Renderer::GetShader("EquirectangularToCubeMap");
		std::shared_ptr<VulkanComputePipeline> equirectangularConversionPipeline = std::make_shared<VulkanComputePipeline>(equirectangularConversionShader);

		Renderer::Submit([equirectangularConversionPipeline, envEquirect, envUnfiltered]()
		{
			auto device = VulkanContext::GetCurrentDevice();
			auto vulkanDescriptorPool = Application::Get()->GetVulkanDescriptorPool();

			VkDescriptorSet equirectSet;
			VulkanDescriptorWriter equirectSetWriter(*equirectangularConversionPipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool);

			VkDescriptorImageInfo cubeMapImageInfo = envUnfiltered->GetDescriptorImageInfo();
			equirectSetWriter.WriteImage(0, &cubeMapImageInfo);

			VkDescriptorImageInfo equirecTexInfo = envEquirect->GetDescriptorImageInfo();
			equirectSetWriter.WriteImage(1, &equirecTexInfo);

			equirectSetWriter.Build(equirectSet);

			VkCommandBuffer dispatchCmd = device->GetCommandBuffer(true);

			vkCmdBindDescriptorSets(dispatchCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				equirectangularConversionPipeline->GetVulkanPipelineLayout(), 0, 1,
				&equirectSet, 0, nullptr);

			equirectangularConversionPipeline->Bind(dispatchCmd);
			equirectangularConversionPipeline->Dispatch(dispatchCmd, cubemapSize / 16, cubemapSize / 16, 6);

			device->FlushCommandBuffer(dispatchCmd, true);
			
			envUnfiltered->GenerateMipMaps(true);
		});

		auto environmentMipFilterShader = Renderer::GetShader("EnvironmentMipFilter");
		std::shared_ptr<VulkanComputePipeline> environmentMipFilterPipeline = std::make_shared<VulkanComputePipeline>(environmentMipFilterShader);

		Renderer::Submit([environmentMipFilterPipeline, envUnfiltered, envFiltered, cubemapSize]
		{
			auto device = VulkanContext::GetCurrentDevice();

			const uint32_t mipCount = std::_Floor_of_log_2(cubemapSize) + 1;
			auto vulkanDescriptorPool = Application::Get()->GetVulkanDescriptorPool();

			// Building Descriptor Sets
			std::vector<VkDescriptorSet> descriptorSets(mipCount);
			std::vector<VulkanDescriptorWriter> descriptorWriter(mipCount,
				{ *environmentMipFilterPipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool });

			for (uint32_t i = 0; i < mipCount; ++i)
			{
				VkDescriptorImageInfo inputTexInfo = envUnfiltered->GetDescriptorImageInfo();
				descriptorWriter[i].WriteImage(1, &inputTexInfo);

				VkDescriptorImageInfo outputTexInfo = envFiltered->GetDescriptorImageInfo();
				outputTexInfo.imageView = envFiltered->CreateImageViewSingleMip(i);
				descriptorWriter[i].WriteImage(0, &outputTexInfo);

				descriptorWriter[i].Build(descriptorSets[i]);
			}

			// Dispatch Pipeline
			VkCommandBuffer dispatchCmd = device->GetCommandBuffer(true);
			environmentMipFilterPipeline->Bind(dispatchCmd);

			const float deltaRoughness = 1.0f / glm::max((float)mipCount - 1.0f, 1.0f);
			for (uint32_t i = 0, size = cubemapSize; i < mipCount; ++i, size /= 2)
			{
				uint32_t numGroups = glm::max(1u, size / 16);
				float roughness = i * deltaRoughness;
				environmentMipFilterPipeline->SetPushConstants(dispatchCmd, &roughness, sizeof(float));
				environmentMipFilterPipeline->Execute(dispatchCmd, descriptorSets[i], numGroups, numGroups, 6);
			}

			device->FlushCommandBuffer(dispatchCmd, true);
		});

		auto environmentIrradianceShader = Renderer::GetShader("EnvironmentIrradiance");
		std::shared_ptr<VulkanComputePipeline> environmentIrradiancePipeline = std::make_shared<VulkanComputePipeline>(environmentIrradianceShader);
		std::shared_ptr<VulkanTextureCube> irradianceMap = std::make_shared<VulkanTextureCube>(irradianceMapSize, irradianceMapSize, ImageFormat::RGBA32F);
		irradianceMap->Invalidate();

		Renderer::Submit([environmentIrradiancePipeline, irradianceMap, envFiltered]
		{
			auto device = VulkanContext::GetCurrentDevice();
			auto vulkanDescriptorPool = Application::Get()->GetVulkanDescriptorPool();

			// Building Descriptor Set
			VkDescriptorSet descriptorSet;
			VulkanDescriptorWriter descriptorWriter(*environmentIrradiancePipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool);

			VkDescriptorImageInfo outputTexInfo = irradianceMap->GetDescriptorImageInfo();
			descriptorWriter.WriteImage(0, &outputTexInfo);

			VkDescriptorImageInfo inputTexInfo = envFiltered->GetDescriptorImageInfo();
			descriptorWriter.WriteImage(1, &inputTexInfo);

			descriptorWriter.Build(descriptorSet);

			// Dispatch Pipeline
			VkCommandBuffer dispatchCmd = device->GetCommandBuffer(true);

			environmentIrradiancePipeline->Bind(dispatchCmd);
			environmentIrradiancePipeline->Execute(dispatchCmd, descriptorSet, irradianceMapSize / 16, irradianceMapSize / 16, 6);

			device->FlushCommandBuffer(dispatchCmd, true);

			irradianceMap->GenerateMipMaps(false);
		});

		return { envFiltered, irradianceMap };
	}

	void VulkanRenderer::CopyVulkanImage(std::shared_ptr<VulkanRenderCommandBuffer> commandBuffer, const VulkanImage* sourceImage, const VulkanImage* destImage)
	{
		Renderer::Submit([commandBuffer, sourceImage, destImage]
		{
			VK_CORE_PROFILE_FN("VulkanRenderer::CopyVulkanImage");
			VkCommandBuffer vulkanCmdBuffer = commandBuffer->RT_GetActiveCommandBuffer();

			VkImage srcImage = sourceImage->GetVulkanImageInfo().Image;
			VkImage dstImage = destImage->GetVulkanImageInfo().Image;

			// TODO: We cannot determine layout like this for image but we are doing this for now to get Bloom
			// Changing Source Image Layout
			Utils::InsertImageMemoryBarrier(vulkanCmdBuffer, srcImage,
				VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

			// Changing Destination Image Layout
			Utils::InsertImageMemoryBarrier(vulkanCmdBuffer, dstImage,
				VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

			VkImageCopy region{};
			region.srcOffset = { 0, 0, 0 };
			region.dstOffset = { 0, 0, 0 };
			region.extent = { sourceImage->GetSpecification().Width, sourceImage->GetSpecification().Height, 1 };
			region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.mipLevel = 0;
			region.srcSubresource.layerCount = 1;
			region.dstSubresource = region.srcSubresource;

			vkCmdCopyImage(vulkanCmdBuffer,
				srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region);

			// Changing source image back to its previous layout
			Utils::InsertImageMemoryBarrier(vulkanCmdBuffer, srcImage,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		});
	}

	void VulkanRenderer::BlitVulkanImage(std::shared_ptr<VulkanRenderCommandBuffer> commandBuffer, const VulkanImage* image)
	{
		Renderer::Submit([commandBuffer, image]
		{
			VK_CORE_PROFILE_FN("VulkanRenderer::BlitVulkanImage");
			VkCommandBuffer vulkanCmdBuffer = commandBuffer->RT_GetActiveCommandBuffer();

			VkImage vulkanImage = image->GetVulkanImageInfo().Image;

			const uint32_t mipLevels = image->GetSpecification().MipLevels;
			const glm::uvec2 imgSize = { image->GetSpecification().Width, image->GetSpecification().Height };

			// Setting Base Mip(Oth) to Source
			VkImageSubresourceRange baseMipSubRange{};
			baseMipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			baseMipSubRange.baseMipLevel = 0;
			baseMipSubRange.baseArrayLayer = 0;
			baseMipSubRange.levelCount = 1;
			baseMipSubRange.layerCount = 1;

			Utils::InsertImageMemoryBarrier(vulkanCmdBuffer, vulkanImage,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				baseMipSubRange);

			// Starting at 1st Mip Level
			for (uint32_t i = 1; i < mipLevels; ++i)
			{
				VkImageBlit imageBlit{};

				// Source
				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcSubresource.baseArrayLayer = 0;
				imageBlit.srcOffsets[1].x = int32_t(imgSize.x >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(imgSize.y >> (i - 1));
				imageBlit.srcOffsets[1].z = 1;

				// Destination
				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstSubresource.baseArrayLayer = 0;
				imageBlit.dstOffsets[1].x = int32_t(imgSize.x >> i);
				imageBlit.dstOffsets[1].y = int32_t(imgSize.y >> i);
				imageBlit.dstOffsets[1].z = 1;

				VkImageSubresourceRange mipSubRange{};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.baseArrayLayer = 0;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				Utils::InsertImageMemoryBarrier(vulkanCmdBuffer, vulkanImage,
					VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					mipSubRange);

				vkCmdBlitImage(vulkanCmdBuffer,
					vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &imageBlit,
					VK_FILTER_LINEAR);

				Utils::InsertImageMemoryBarrier(vulkanCmdBuffer, vulkanImage,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					mipSubRange);
			}

			VkImageSubresourceRange subresourceRange{};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.layerCount = 1;
			subresourceRange.levelCount = mipLevels;

			Utils::InsertImageMemoryBarrier(vulkanCmdBuffer, vulkanImage,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);
		});
	}		
	
	std::shared_ptr<VulkanImage> VulkanRenderer::CreateBRDFTexture()
	{
		constexpr uint32_t textureSize = 512;

		ImageSpecification brdfTextureSpec;
		brdfTextureSpec.Width = textureSize;
		brdfTextureSpec.Height = textureSize;
		brdfTextureSpec.Usage = ImageUsage::Storage;
		brdfTextureSpec.Format = ImageFormat::RGBA32F;
		brdfTextureSpec.SamplerWrap = TextureWrap::Clamp;

		auto generateBRDFShader = Renderer::GetShader("GenerateBRDF");
		std::shared_ptr<VulkanComputePipeline> generateBRDFPipeline = std::make_shared<VulkanComputePipeline>(generateBRDFShader);
		std::shared_ptr<VulkanImage> brdfTexture = std::make_shared<VulkanImage>(brdfTextureSpec);
		brdfTexture->Invalidate();

		Renderer::Submit([generateBRDFPipeline, brdfTexture, textureSize]
		{
			auto device = VulkanContext::GetCurrentDevice();
			auto vulkanDescriptorPool = Application::Get()->GetVulkanDescriptorPool();

			// Building Descriptor Set
			VkDescriptorSet descriptorSet;
			VulkanDescriptorWriter descriptorWriter(*generateBRDFPipeline->GetDescriptorSetLayout(), *vulkanDescriptorPool);

			VkDescriptorImageInfo brdfOutputInfo = brdfTexture->GetDescriptorInfo();
			descriptorWriter.WriteImage(0, &brdfOutputInfo);

			descriptorWriter.Build(descriptorSet);

			// Dispatch Pipeline
			VkCommandBuffer dispatchCmd = device->GetCommandBuffer(true);

			generateBRDFPipeline->Bind(dispatchCmd);
			generateBRDFPipeline->Execute(dispatchCmd, descriptorSet, textureSize / 16, textureSize / 16, 1);

			device->FlushCommandBuffer(dispatchCmd, true);
		});

		return brdfTexture;
	}

	void VulkanRenderer::RenderMesh(std::shared_ptr<VulkanRenderCommandBuffer> cmdBuffer, std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, uint32_t submeshIndex, std::shared_ptr<VulkanPipeline> pipeline, std::shared_ptr<VulkanVertexBuffer> transformBuffer, const std::vector<TransformData>& transformData, uint32_t instanceCount)
	{
		Renderer::Submit([cmdBuffer, mesh, pipeline, material, transformBuffer, transformData, submeshIndex, instanceCount]
		{
			VK_CORE_PROFILE_FN("VulkanRenderer::RenderMesh");

			// Bind Vertex Buffer
			auto drawCmd = cmdBuffer->RT_GetActiveCommandBuffer();

			auto meshSource = mesh->GetMeshSource();
			transformBuffer->WriteData((void*)transformData.data(), 0);
			VkBuffer buffers[] = { meshSource->GetVertexBuffer()->GetVulkanBuffer(), transformBuffer->GetVulkanBuffer() };
			VkDeviceSize offsets[] = { 0, 0 };
			vkCmdBindVertexBuffers(drawCmd, 0, 2, buffers, offsets);
			vkCmdBindIndexBuffer(drawCmd, meshSource->GetIndexBuffer()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);

			std::shared_ptr<VulkanMaterial> vulkanMaterial = std::static_pointer_cast<VulkanMaterial>(material);
			VkDescriptorSet descriptorSets[1] = { vulkanMaterial->RT_GetVulkanMaterialDescriptorSet() };
	
			vkCmdBindDescriptorSets(drawCmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipeline->GetVulkanPipelineLayout(),
				1, 1, descriptorSets,
				0, nullptr);
	
			vkCmdPushConstants(drawCmd,
				pipeline->GetVulkanPipelineLayout(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(MaterialData),
				&material->GetMaterialData());
	
			const auto& submeshes = mesh->GetMeshSource()->GetSubmeshes();
			const Submesh& submesh = submeshes[submeshIndex];
			vkCmdDrawIndexed(drawCmd, submesh.IndexCount, instanceCount, submesh.BaseIndex, submesh.BaseVertex, 0);

			s_Data.DrawCalls++;
			s_Data.InstanceCount += instanceCount;
		});
	}

	void VulkanRenderer::ResetStats()
	{
		s_Data.DrawCalls = 0;
		s_Data.InstanceCount = 0;
	}

	void VulkanRenderer::CreateCommandBuffers()
	{
		auto device = VulkanContext::GetCurrentDevice();

		m_CommandBuffer = std::make_shared<VulkanRenderCommandBuffer>(device->GetRenderThreadCommandPool());
	}

	void VulkanRenderer::RecreateSwapChain()
	{
		auto device = VulkanContext::GetCurrentDevice();
		auto extent = m_Window->GetExtent();

		while (extent.width == 0 || extent.height == 0)
		{
			extent = m_Window->GetExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device->GetVulkanDevice());
		m_SwapChain.reset();

		if (m_SwapChain == nullptr)
		{
			m_SwapChain = std::make_unique<VulkanSwapChain>(extent);
		}

		else
		{
			std::shared_ptr<VulkanSwapChain> oldSwapChain = std::move(m_SwapChain);
			m_SwapChain = std::make_unique<VulkanSwapChain>(extent, oldSwapChain);

			if (!oldSwapChain->CompareSwapFormats(*m_SwapChain->GetSwapChain()))
			{
				VK_CORE_ASSERT(false, "Swap Chain Image(or Depth) Format has changed!");
			}
		}

	}

	void VulkanRenderer::FinalQueueSubmit()
	{
		Renderer::Submit([this]
		{
			VK_CORE_PROFILE_FN("VulkanRenderer::FinalQueueSubmit");

			auto sceneRenderer = SceneRenderer::GetSceneRenderer();

			const std::vector<VkCommandBuffer> cmdBuffers{ m_CommandBuffer->RT_GetActiveCommandBuffer(), sceneRenderer->GetCommandBuffer()->RT_GetActiveCommandBuffer() };
			auto result = m_SwapChain->SubmitCommandBuffers(cmdBuffers, &m_CurrentImageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window->IsWindowResize())
			{
				m_Window->ResetWindowResizeFlag();
				RecreateSwapChain();
				sceneRenderer->RecreateScene();
			}

			else if (result != VK_SUCCESS)
				VK_CORE_ERROR("Failed to Present Swap Chain Image!");
		});

		Renderer::WaitAndRender();

		IsFrameStarted = false;
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % VulkanSwapChain::MaxFramesInFlight;
	}

	void VulkanRenderer::FinalQueueSubmit(const std::vector<VkCommandBuffer>& cmdBuffers)
	{
		auto result = m_SwapChain->SubmitCommandBuffers(cmdBuffers, &m_CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window->IsWindowResize())
		{
			m_Window->ResetWindowResizeFlag();
			RecreateSwapChain();
			SceneRenderer::GetSceneRenderer()->RecreateScene();
		}

		else if (result != VK_SUCCESS)
			VK_CORE_ERROR("Failed to Present Swap Chain Image!");

		IsFrameStarted = false;
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % VulkanSwapChain::MaxFramesInFlight;
	}

}