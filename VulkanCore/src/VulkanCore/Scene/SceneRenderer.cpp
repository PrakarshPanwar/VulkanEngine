#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/TextureImporter.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "VulkanCore/Renderer/Renderer.h"

#include "VulkanCore/Mesh/Mesh.h"
#include "Platform/Vulkan/VulkanAllocator.h"
#include "Platform/Vulkan/VulkanMaterial.h"
#include "Platform/Vulkan/VulkanFramebuffer.h"
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "Platform/Vulkan/VulkanComputePipeline.h"
#include "Platform/Vulkan/VulkanIndexBuffer.h"
#include "Platform/Vulkan/VulkanUniformBuffer.h"
#include "Platform/Vulkan/VulkanStorageBuffer.h"

#include <imgui.h>

namespace VulkanCore {

	namespace Utils {

		std::shared_ptr<VulkanVertexBuffer> CreateCubeModel()
		{
			float skyboxVertices[] = {       
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};

			return std::make_shared<VulkanVertexBuffer>(skyboxVertices, (uint32_t)sizeof(skyboxVertices));
		}

		static uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::_Floor_of_log_2(std::max(width, height)) + 1;
		}

		static VkDeviceAddress GetBufferDeviceAddress(VkBuffer buffer)
		{
			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
			bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			bufferDeviceAddressInfo.buffer = buffer;
			
			return vkGetBufferDeviceAddressKHR(vulkanDevice, &bufferDeviceAddressInfo);
		}

	}

	SceneRenderer* SceneRenderer::s_Instance = nullptr;

	SceneRenderer::SceneRenderer(std::shared_ptr<Scene> scene)
		: m_Scene(scene), m_RayTraced(Application::Get()->GetSpecification().RayTracing)
	{
		s_Instance = this;
		Init();
	}

	SceneRenderer::~SceneRenderer()
	{
		Release();
	}

	void SceneRenderer::Init()
	{
		CreateCommandBuffers();
		CreatePipelines();
		CreateResources();
		CreateMaterials();
	}

	// This one will be called after we have serialized our scene
	void SceneRenderer::CreateRayTraceResources()
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		std::vector<MeshBuffersData> meshBuffersData;
		for (auto& [mk, tc] : m_MeshTraceList)
		{
			// Submit Mesh Buffers Data
			auto meshSource = tc.MeshInstance->GetMeshSource();

			auto vulkanMeshVB = std::static_pointer_cast<VulkanVertexBuffer>(meshSource->GetVertexBuffer());
			auto vulkanMeshIB = std::static_pointer_cast<VulkanIndexBuffer>(meshSource->GetIndexBuffer());

			auto& bufferData = meshBuffersData.emplace_back();
			bufferData.VertexBufferAddress = Utils::GetBufferDeviceAddress(vulkanMeshVB->GetVulkanBuffer());
			bufferData.IndexBufferAddress = Utils::GetBufferDeviceAddress(vulkanMeshIB->GetVulkanBuffer());

			// Submit Mesh Data to AS
			m_SceneAccelerationStructure->SubmitMeshDrawData(tc.MeshInstance, tc.MaterialInstance, m_MeshTransformMap[mk].Transforms, tc.SubmeshIndex, tc.InstanceCount);
		}

		m_SceneAccelerationStructure->BuildBottomLevelAccelerationStructures();
		m_SceneAccelerationStructure->BuildTopLevelAccelerationStructure();

		// Write Data in Storage Buffers
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto vulkanSBMeshBuffersData = std::make_shared<VulkanStorageBuffer>(sizeof(MeshBuffersData) * m_MeshTraceList.size());
			m_SBMeshBuffersData.emplace_back(vulkanSBMeshBuffersData);

			// Set Debug Names
			VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(),
				VK_OBJECT_TYPE_BUFFER,
				std::format("Mesh Data Storage Buffer: {}", i),
				vulkanSBMeshBuffersData->GetDescriptorBufferInfo().buffer);

			vulkanSBMeshBuffersData->WriteAndFlushBuffer(meshBuffersData.data(), 0);
		}

		m_RayTracingMaterial->SetAccelerationStructure(0, m_SceneAccelerationStructure);
		m_RayTracingMaterial->SetBuffers(4, m_SBMeshBuffersData);
		m_RayTracingMaterial->SetTexture(5, m_CubemapTexture);
		m_RayTracingMaterial->PrepareShaderMaterial();
	}

	void SceneRenderer::CreatePipelines()
	{
		VertexBufferLayout vertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float3, "a_FragColor" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};

		VertexBufferLayout instanceLayout = {
			{ ShaderDataType::Float4, "a_MRow0" },
			{ ShaderDataType::Float4, "a_MRow1" },
			{ ShaderDataType::Float4, "a_MRow2" }
		};

		// Geometry and Point Light Pipeline
		{
			FramebufferSpecification geomFramebufferSpec;
			geomFramebufferSpec.Width = 1920;
			geomFramebufferSpec.Height = 1080;
			geomFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::DEPTH24STENCIL8 };
			geomFramebufferSpec.Transfer = true;
			geomFramebufferSpec.Samples = 8;

			RenderPassSpecification geomRenderPassSpec;
			geomRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(geomFramebufferSpec);

			PipelineSpecification geomPipelineSpec;
			geomPipelineSpec.DebugName = "Geometry Pipeline";
			geomPipelineSpec.pShader = Renderer::GetShader("CorePBR");
			geomPipelineSpec.Blend = true;
			geomPipelineSpec.pRenderPass = std::make_shared<VulkanRenderPass>(geomRenderPassSpec);
			geomPipelineSpec.Layout = vertexLayout;
			geomPipelineSpec.InstanceLayout = instanceLayout;

			PipelineSpecification lightPipelineSpec;
			lightPipelineSpec.DebugName = "Light Pipeline";
			lightPipelineSpec.pShader = Renderer::GetShader("LightShader");
			lightPipelineSpec.Blend = true;
			lightPipelineSpec.pRenderPass = geomPipelineSpec.pRenderPass;

			m_GeometryPipeline = std::make_shared<VulkanPipeline>(geomPipelineSpec);
			m_LightPipeline = std::make_shared<VulkanPipeline>(lightPipelineSpec);
		}

		// Composite Pipeline
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.Width = 1920;
			compFramebufferSpec.Height = 1080;
			compFramebufferSpec.Attachments = { ImageFormat::RGBA8_SRGB };
			compFramebufferSpec.Samples = 1;

			RenderPassSpecification compRenderPassSpec;
			compRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(compFramebufferSpec);
			m_SceneFramebuffer = compRenderPassSpec.TargetFramebuffer;

			PipelineSpecification compPipelineSpec;
			compPipelineSpec.DebugName = "Composite Pipeline";
			compPipelineSpec.pShader = Renderer::GetShader("SceneComposite");
			compPipelineSpec.pRenderPass = std::make_shared<VulkanRenderPass>(compRenderPassSpec);
			compPipelineSpec.DepthTest = false;
			compPipelineSpec.DepthWrite = false;

			m_CompositePipeline = std::make_shared<VulkanPipeline>(compPipelineSpec);
		}

		// Skybox Pipeline
		{
			PipelineSpecification skyboxPipelineSpec;
			skyboxPipelineSpec.DebugName = "Skybox Pipeline";
			skyboxPipelineSpec.pShader = Renderer::GetShader("Skybox");
			skyboxPipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" }
			};

			skyboxPipelineSpec.pRenderPass = m_GeometryPipeline->GetSpecification().pRenderPass;

			m_SkyboxPipeline = std::make_shared<VulkanPipeline>(skyboxPipelineSpec);
		}

		// Bloom Pipeline
		{
			m_BloomPipeline = std::make_shared<VulkanComputePipeline>(Renderer::GetShader("Bloom"), "Bloom Pipeline");
		}

		// Ray Tracing Pipeline
		{
			m_RayTracingPipeline = std::make_shared<VulkanRayTracingPipeline>(Renderer::GetShader("CoreRT"), "Ray Tracing Pipeline");
			m_RayTracingPipeline->CreateShaderBindingTable();
		}
	}

	void SceneRenderer::CreateMaterials()
	{
		// Geometry Material
		{
			m_GeometryMaterial = std::make_shared<VulkanMaterial>(m_GeometryPipeline->GetSpecification().pShader, "Geometry Shader Material");

			m_GeometryMaterial->SetBuffers(0, m_UBCamera);
			m_GeometryMaterial->SetBuffers(1, m_UBPointLight);
			m_GeometryMaterial->SetBuffers(2, m_UBSpotLight);
			m_GeometryMaterial->SetTexture(6, m_IrradianceTexture);
			m_GeometryMaterial->SetImage(7, m_BRDFTexture);
			m_GeometryMaterial->SetTexture(8, m_PrefilteredTexture);
			m_GeometryMaterial->PrepareShaderMaterial();
		}

		// Light Materials
		{
			m_PointLightShaderMaterial = std::make_shared<VulkanMaterial>(m_LightPipeline->GetSpecification().pShader, "Point Light Shader Material");
			m_SpotLightShaderMaterial = std::make_shared<VulkanMaterial>(m_LightPipeline->GetSpecification().pShader, "Spot Light Shader Material");

			m_PointLightShaderMaterial->SetBuffers(0, m_UBCamera);
			m_PointLightShaderMaterial->SetTexture(1, std::dynamic_pointer_cast<VulkanTexture>(m_PointLightTextureIcon));
			m_PointLightShaderMaterial->PrepareShaderMaterial();

			m_SpotLightShaderMaterial->SetBuffers(0, m_UBCamera);
			m_SpotLightShaderMaterial->SetTexture(1, std::dynamic_pointer_cast<VulkanTexture>(m_SpotLightTextureIcon));
			m_SpotLightShaderMaterial->PrepareShaderMaterial();
		}

		// Bloom Materials
		{
			const uint32_t mipCount = m_BloomTextures[0]->GetSpecification().MipLevels;
			auto bloomTexture0 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[0]);
			auto bloomTexture1 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[1]);
			auto bloomTexture2 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[2]);

			// Set A: Prefiltering
			// Binding 0(o_Image): BloomTex[0]
			// Binding 1(u_Texture): RenderTex
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomPrefilterShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Prefilter Shader Material");

			m_BloomPrefilterShaderMaterial->SetImage(0, m_BloomTextures[0]);
			m_BloomPrefilterShaderMaterial->SetImages(1, m_SceneRenderTextures);
			m_BloomPrefilterShaderMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomPrefilterShaderMaterial->PrepareShaderMaterial();

			m_BloomPingShaderMaterials.resize(mipCount - 1);
			m_BloomPongShaderMaterials.resize(mipCount - 1);
			for (uint32_t i = 1; i < mipCount; ++i)
			{
				// Set B: Downsampling(Ping)
				// Binding 0(o_Image): BloomTex[1]
				// Binding 1(u_Texture): BloomTex[0]
				// Binding 2(u_BloomTexture): RenderTex
				auto bloomPingShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Ping Shader Material");

				bloomTexture1->CreateImageViewSingleMip(i);
				bloomPingShaderMaterial->SetImage(0, m_BloomTextures[1], i);
				bloomPingShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomPingShaderMaterial->SetImages(2, m_SceneRenderTextures);
				bloomPingShaderMaterial->PrepareShaderMaterial();

				m_BloomPingShaderMaterials[i - 1] = bloomPingShaderMaterial;

				// Set C: Downsampling(Pong)
				// Binding 0(o_Image): BloomTex[0]
				// Binding 1(u_Texture): BloomTex[1]
				// Binding 2(u_BloomTexture): RenderTex
				auto bloomPongShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Pong Shader Material");

				bloomTexture0->CreateImageViewSingleMip(i);
				bloomPongShaderMaterial->SetImage(0, m_BloomTextures[0], i);
				bloomPongShaderMaterial->SetImage(1, m_BloomTextures[1]);
				bloomPongShaderMaterial->SetImages(2, m_SceneRenderTextures);
				bloomPongShaderMaterial->PrepareShaderMaterial();

				m_BloomPongShaderMaterials[i - 1] = bloomPongShaderMaterial;
			}

			// Set D: First Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomUpsampleFirstShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample First Shader Material");

			bloomTexture2->CreateImageViewSingleMip(mipCount - 1);
			m_BloomUpsampleFirstShaderMaterial->SetImage(0, m_BloomTextures[2], mipCount - 1);
			m_BloomUpsampleFirstShaderMaterial->SetImage(1, m_BloomTextures[0]);
			m_BloomUpsampleFirstShaderMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomUpsampleFirstShaderMaterial->PrepareShaderMaterial();

			// Set E: Final Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): BloomTex[2]
			m_BloomUpsampleShaderMaterials.resize(mipCount - 1);
			for (int i = mipCount - 2; i >= 0; --i)
			{
				auto bloomUpsampleShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample Shader Material");

				bloomTexture2->CreateImageViewSingleMip(i);
				bloomUpsampleShaderMaterial->SetImage(0, m_BloomTextures[2], i);
				bloomUpsampleShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomUpsampleShaderMaterial->SetImage(2, m_BloomTextures[2]);
				bloomUpsampleShaderMaterial->PrepareShaderMaterial();

				m_BloomUpsampleShaderMaterials[i] = bloomUpsampleShaderMaterial;
			}
		}

		// Composite Material
		{
			m_CompositeShaderMaterial = std::make_shared<VulkanMaterial>(m_CompositePipeline->GetSpecification().pShader, "Composite Shader Material");

			auto geomFB = std::dynamic_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);
			m_CompositeShaderMaterial->SetImages(0, geomFB->GetAttachment(true));
			m_CompositeShaderMaterial->SetImage(1, m_BloomTextures[2]);
			m_CompositeShaderMaterial->SetTexture(2, std::dynamic_pointer_cast<VulkanTexture>(m_BloomDirtTexture));
			m_CompositeShaderMaterial->PrepareShaderMaterial();
		}

		// Skybox Material
		{
			m_SkyboxMaterial = std::make_shared<VulkanMaterial>(m_SkyboxPipeline->GetSpecification().pShader, "Skybox Shader Material");

			m_SkyboxMaterial->SetBuffers(0, m_UBCamera);
			m_SkyboxMaterial->SetTexture(1, m_CubemapTexture);
			m_SkyboxMaterial->PrepareShaderMaterial();
		}

		// Ray Tracing Material
		{
			m_RayTracingMaterial = std::make_shared<VulkanMaterial>(m_RayTracingPipeline->GetShader(), "Ray Tracing Shader Material");

			m_RayTracingMaterial->SetImages(1, m_SceneRTOutputImages);
			m_RayTracingMaterial->SetBuffers(2, m_UBCamera);
			m_RayTracingMaterial->SetBuffers(3, m_UBPointLight);
			m_RayTracingMaterial->PrepareShaderMaterial();
		}
	}

	void SceneRenderer::RecreateMaterials()
	{
		// Bloom Materials
		{
			const uint32_t mipCount = m_BloomTextures[0]->GetSpecification().MipLevels;
			auto bloomTexture0 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[0]);
			auto bloomTexture1 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[1]);
			auto bloomTexture2 = std::static_pointer_cast<VulkanImage>(m_BloomTextures[2]);

			// Set A: Prefiltering
			// Binding 0(o_Image): BloomTex[0]
			// Binding 1(u_Texture): RenderTex
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomPrefilterShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Prefilter Shader Material");

			m_BloomPrefilterShaderMaterial->SetImage(0, m_BloomTextures[0]);
			m_BloomPrefilterShaderMaterial->SetImages(1, m_SceneRenderTextures);
			m_BloomPrefilterShaderMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomPrefilterShaderMaterial->PrepareShaderMaterial();

			m_BloomPingShaderMaterials.resize(mipCount - 1);
			m_BloomPongShaderMaterials.resize(mipCount - 1);
			for (uint32_t i = 1; i < mipCount; ++i)
			{
				// Set B: Downsampling(Ping)
				// Binding 0(o_Image): BloomTex[1]
				// Binding 1(u_Texture): BloomTex[0]
				// Binding 2(u_BloomTexture): RenderTex
				auto bloomPingShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Ping Shader Material");

				bloomTexture1->CreateImageViewSingleMip(i);
				bloomPingShaderMaterial->SetImage(0, m_BloomTextures[1], i);
				bloomPingShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomPingShaderMaterial->SetImages(2, m_SceneRenderTextures);
				bloomPingShaderMaterial->PrepareShaderMaterial();

				m_BloomPingShaderMaterials[i - 1] = bloomPingShaderMaterial;

				// Set C: Downsampling(Pong)
				// Binding 0(o_Image): BloomTex[0]
				// Binding 1(u_Texture): BloomTex[1]
				// Binding 2(u_BloomTexture): RenderTex
				auto bloomPongShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Pong Shader Material");

				bloomTexture0->CreateImageViewSingleMip(i);
				bloomPongShaderMaterial->SetImage(0, m_BloomTextures[0], i);
				bloomPongShaderMaterial->SetImage(1, m_BloomTextures[1]);
				bloomPongShaderMaterial->SetImages(2, m_SceneRenderTextures);
				bloomPongShaderMaterial->PrepareShaderMaterial();

				m_BloomPongShaderMaterials[i - 1] = bloomPongShaderMaterial;
			}

			// Set D: First Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): RenderTex
			m_BloomUpsampleFirstShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample First Shader Material");

			bloomTexture2->CreateImageViewSingleMip(mipCount - 1);
			m_BloomUpsampleFirstShaderMaterial->SetImage(0, m_BloomTextures[2], mipCount - 1);
			m_BloomUpsampleFirstShaderMaterial->SetImage(1, m_BloomTextures[0]);
			m_BloomUpsampleFirstShaderMaterial->SetImages(2, m_SceneRenderTextures);
			m_BloomUpsampleFirstShaderMaterial->PrepareShaderMaterial();

			// Set E: Final Upsampling
			// Binding 0(o_Image): BloomTex[2]
			// Binding 1(u_Texture): BloomTex[0]
			// Binding 2(u_BloomTexture): BloomTex[2]
			m_BloomUpsampleShaderMaterials.resize(mipCount - 1);
			for (int i = mipCount - 2; i >= 0; --i)
			{
				auto bloomUpsampleShaderMaterial = std::make_shared<VulkanMaterial>(m_BloomPipeline->GetShader(), "Bloom Upsample Shader Material");

				bloomTexture2->CreateImageViewSingleMip(i);
				bloomUpsampleShaderMaterial->SetImage(0, m_BloomTextures[2], i);
				bloomUpsampleShaderMaterial->SetImage(1, m_BloomTextures[0]);
				bloomUpsampleShaderMaterial->SetImage(2, m_BloomTextures[2]);
				bloomUpsampleShaderMaterial->PrepareShaderMaterial();

				m_BloomUpsampleShaderMaterials[i] = bloomUpsampleShaderMaterial;
			}
		}

		// Composite Material
		{
			auto geomFB = std::dynamic_pointer_cast<VulkanFramebuffer>(m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);

			m_CompositeShaderMaterial->SetImages(0, geomFB->GetAttachment(true));
			m_CompositeShaderMaterial->SetImage(1, m_BloomTextures[2]);
			m_CompositeShaderMaterial->SetTexture(2, std::dynamic_pointer_cast<VulkanTexture>(m_BloomDirtTexture));
			m_CompositeShaderMaterial->PrepareShaderMaterial();
		}

		// Skybox Material
		{
			m_SkyboxMaterial->SetBuffers(0, m_UBCamera);
			m_SkyboxMaterial->SetTexture(1, m_CubemapTexture);
			m_SkyboxMaterial->PrepareShaderMaterial();
		}

		// Ray Tracing Material
		{
			m_RayTracingMaterial->SetImages(1, m_SceneRTOutputImages);
			m_RayTracingMaterial->PrepareShaderMaterial();
		}
	}

	void SceneRenderer::RecreatePipelines()
	{
		m_GeometryPipeline->ReloadPipeline();
		m_LightPipeline->ReloadPipeline();
		m_CompositePipeline->ReloadPipeline();
		m_SkyboxPipeline->ReloadPipeline();
		m_BloomPipeline->ReloadPipeline();
	}

	void SceneRenderer::CreateResources()
	{
		auto device = VulkanContext::GetCurrentDevice();

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Renderer::WaitAndExecute();

		m_UBCamera.reserve(framesInFlight);
		m_UBPointLight.reserve(framesInFlight);
		m_UBSpotLight.reserve(framesInFlight);
		m_SBMeshBuffersData.reserve(framesInFlight);

		// Uniform Buffers
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			m_UBCamera.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBCamera)));
			m_UBPointLight.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBPointLights)));
			m_UBSpotLight.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBSpotLights)));
		}

		m_BloomMipSize = (glm::uvec2(m_ViewportSize.x, m_ViewportSize.y) + 1u) / 2u;
		m_BloomMipSize += 16u - m_BloomMipSize % 16u;

		ImageSpecification bloomRTSpec = {};
		bloomRTSpec.DebugName = "Bloom Compute Texture";
		bloomRTSpec.Width = m_BloomMipSize.x;
		bloomRTSpec.Height = m_BloomMipSize.y;
		bloomRTSpec.Format = ImageFormat::RGBA32F;
		bloomRTSpec.Usage = ImageUsage::Storage;
		bloomRTSpec.MipLevels = Utils::CalculateMipCount(m_BloomMipSize.x, m_BloomMipSize.y) - 2;

		m_BloomTextures.reserve(framesInFlight);
		m_SceneRenderTextures.reserve(framesInFlight);

		VkCommandBuffer barrierCmd = device->GetCommandBuffer();

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto BloomTexture = std::static_pointer_cast<VulkanImage>(m_BloomTextures.emplace_back(std::make_shared<VulkanImage>(bloomRTSpec)));
			BloomTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, BloomTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, BloomTexture->GetSpecification().MipLevels, 0, 1 });
		}

		ImageSpecification sceneRenderTextureSpec = {};
		sceneRenderTextureSpec.DebugName = "Scene Render Texture";
		sceneRenderTextureSpec.Width = m_ViewportSize.x;
		sceneRenderTextureSpec.Height = m_ViewportSize.y;
		sceneRenderTextureSpec.Format = ImageFormat::RGBA32F;
		sceneRenderTextureSpec.Usage = ImageUsage::Texture;
		sceneRenderTextureSpec.MipLevels = Utils::CalculateMipCount(m_ViewportSize.x, m_ViewportSize.y);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto SceneTexture = std::static_pointer_cast<VulkanImage>(m_SceneRenderTextures.emplace_back(std::make_shared<VulkanImage>(sceneRenderTextureSpec)));
			SceneTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, SceneTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneTexture->GetSpecification().MipLevels, 0, 1 });
		}

		ImageSpecification sceneRTOutputSpec = {};
		sceneRTOutputSpec.DebugName = "Scene Ray Tracing Output";
		sceneRTOutputSpec.Width = m_ViewportSize.x;
		sceneRTOutputSpec.Height = m_ViewportSize.y;
		// TODO: Render in HDR Format then Tonemap in post(when material system is supported in Ray Tracing Pipeline)
		sceneRTOutputSpec.Format = ImageFormat::RGBA32F;
		sceneRTOutputSpec.Usage = ImageUsage::Storage;
		sceneRTOutputSpec.Transfer = true;

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto SceneRTOutputTexture = std::static_pointer_cast<VulkanImage>(m_SceneRTOutputImages.emplace_back(std::make_shared<VulkanImage>(sceneRTOutputSpec)));
			SceneRTOutputTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, SceneRTOutputTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneRTOutputTexture->GetSpecification().MipLevels, 0, 1 });
		}

		device->FlushCommandBuffer(barrierCmd);

		m_SceneImages.resize(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			std::shared_ptr<VulkanImage> finalPassImage = std::dynamic_pointer_cast<VulkanImage>(GetFinalPassImage(i));
			m_SceneImages[i] = ImGuiLayer::AddTexture(*finalPassImage);
		}

		m_BloomDirtTexture = AssetManager::GetAsset<Texture2D>("assets/textures/LensDirt.png");

		auto blackTextureCube = std::dynamic_pointer_cast<VulkanTextureCube>(Renderer::GetBlackTextureCube(ImageFormat::RGBA8_UNORM));
		blackTextureCube->Invalidate();

		m_CubemapTexture = blackTextureCube;
		m_PrefilteredTexture = blackTextureCube;
		m_IrradianceTexture = blackTextureCube;

		m_SkyboxTextureID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTextureCube>(m_PrefilteredTexture));

		m_BRDFTexture = Renderer::CreateBRDFTexture();
		m_PointLightTextureIcon = TextureImporter::LoadTexture2D("../EditorLayer/Resources/Icons/PointLightIcon.png");
		m_SpotLightTextureIcon = TextureImporter::LoadTexture2D("../EditorLayer/Resources/Icons/SpotLightIcon.png");

		m_SceneAccelerationStructure = std::make_shared<VulkanAccelerationStructure>();

		m_SkyboxVBData = Utils::CreateCubeModel();
	}

	void SceneRenderer::Release()
	{
	}

	void SceneRenderer::RecreateScene()
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VK_CORE_INFO("Scene has been Recreated!");
		m_GeometryPipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
		m_CompositePipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);

		// Recreate Resources
		{
			m_BloomMipSize = (glm::uvec2(m_ViewportSize.x, m_ViewportSize.y) + 1u) / 2u;
			m_BloomMipSize += 16u - m_BloomMipSize % 16u;

			VkCommandBuffer barrierCmd = device->GetCommandBuffer();

			for (auto& BloomTexture : m_BloomTextures)
			{
				auto vulkanBloomTexture = std::dynamic_pointer_cast<VulkanImage>(BloomTexture);
				vulkanBloomTexture->Resize(m_BloomMipSize.x, m_BloomMipSize.y, Utils::CalculateMipCount(m_BloomMipSize.x, m_BloomMipSize.y) - 2);

				Utils::InsertImageMemoryBarrier(barrierCmd, vulkanBloomTexture->GetVulkanImageInfo().Image,
					VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, BloomTexture->GetSpecification().MipLevels, 0, 1 });
			}

			for (auto& SceneRenderTexture : m_SceneRenderTextures)
			{
				auto vulkanSceneTexture = std::dynamic_pointer_cast<VulkanImage>(SceneRenderTexture);
				vulkanSceneTexture->Resize(m_ViewportSize.x, m_ViewportSize.y, Utils::CalculateMipCount(m_ViewportSize.x, m_ViewportSize.y));

				Utils::InsertImageMemoryBarrier(barrierCmd, vulkanSceneTexture->GetVulkanImageInfo().Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneRenderTexture->GetSpecification().MipLevels, 0, 1 });
			}

			for (auto& SceneRTOutputTexture : m_SceneRTOutputImages)
			{
				auto vulkanRTOutputImage = std::static_pointer_cast<VulkanImage>(SceneRTOutputTexture);
				vulkanRTOutputImage->Resize(m_ViewportSize.x, m_ViewportSize.y);

				Utils::InsertImageMemoryBarrier(barrierCmd, vulkanRTOutputImage->GetVulkanImageInfo().Image,
					VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneRTOutputTexture->GetSpecification().MipLevels, 0, 1 });
			}

			device->FlushCommandBuffer(barrierCmd);

			// Update ImGui Viewport Image
			for (uint32_t i = 0; i < framesInFlight; ++i)
			{
				std::shared_ptr<VulkanImage> finalPassImage = std::dynamic_pointer_cast<VulkanImage>(GetFinalPassImage(i));
				ImGuiLayer::UpdateDescriptor(m_SceneImages[i], *finalPassImage);
			}
		}

		// Recreate Shader Materials
		RecreateMaterials();
	}

	void SceneRenderer::OnImGuiRender()
	{
		ImGui::Begin("Scene Renderer");

		const ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (ImGui::TreeNodeEx("Scene Settings", treeFlags))
		{
			ImGui::DragFloat("Exposure Intensity", &m_SceneSettings.Exposure, 0.01f, 0.0f, 20.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragFloat("Dirt Intensity", &m_SceneSettings.DirtIntensity, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("Skybox LOD", &m_SkyboxSettings.LOD, 0.01f, 0.0f, 11.0f);
			ImGui::DragFloat("Skybox Intensity", &m_SkyboxSettings.Intensity, 0.01f, 0.0f, 20.0f);

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene Renderer Stats##GPUPerf", treeFlags))
		{
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);

			if (m_RayTraced)
			{
				ImGui::Text("RayTrace Pass: %lluns", vulkanCmdBuffer->GetQueryTime(0));
				ImGui::Text("Composite Pass: %lluns", vulkanCmdBuffer->GetQueryTime(2));
				ImGui::Text("Bloom Compute Pass: %lluns", vulkanCmdBuffer->GetQueryTime(3));
			}
			else
			{
				ImGui::Text("Geometry Pass: %lluns", vulkanCmdBuffer->GetQueryTime(1));
				ImGui::Text("Skybox Pass: %lluns", vulkanCmdBuffer->GetQueryTime(0));
				ImGui::Text("Lights Pass: %lluns", vulkanCmdBuffer->GetQueryTime(2));
				ImGui::Text("Composite Pass: %lluns", vulkanCmdBuffer->GetQueryTime(4));
				ImGui::Text("Bloom Compute Pass: %lluns", vulkanCmdBuffer->GetQueryTime(3));
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Bloom Settings", treeFlags))
		{
			ImGui::DragFloat("Threshold", &m_BloomParams.Threshold, 0.01f, 0.0f, 1000.0f);
			ImGui::DragFloat("Knee", &m_BloomParams.Knee, 0.01f, 0.001f, 1.0f);
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene Shader Map", treeFlags))
		{
			auto& shaderMap = Renderer::GetShaderMap();

			int buttonID = 0;
			for (auto&& [name, shader] : shaderMap)
			{
				ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2{ 0.0f, 24.5f });
				ImGui::SetItemAllowOverlap();

				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
				ImGui::PushID(buttonID++);
				if (ImGui::Button("Reload", ImVec2{ 0.0f, 24.5f }))
				{
					shader->Reload();
					RecreatePipelines();
				}

				ImGui::PopID();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene Draw Stats", treeFlags))
		{
			auto renderStats = VulkanRenderer::GetRendererStats();

			ImGui::Text("Draw Calls: %u", renderStats.DrawCalls);
			ImGui::Text("Instance Count: %u", renderStats.InstanceCount);
			ImGui::Text("Draw Calls Saved: %u", renderStats.InstanceCount - renderStats.DrawCalls);
			ImGui::TreePop();
		}

		ImGui::End(); // End of Scene Renderer Window

		VulkanRenderer::ResetStats();
	}

	void SceneRenderer::SetActiveScene(std::shared_ptr<Scene> scene)
	{
		m_Scene = scene;

		//m_MeshDrawList.clear();
		//m_MeshTransformMap.clear();
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportSize.x = width;
		m_ViewportSize.y = height;

		RecreateScene();
	}

	void SceneRenderer::RasterizeScene()
	{
		VK_CORE_PROFILE_FN("Submit-SceneRenderer");

		m_SceneCommandBuffer->Begin();

		GeometryPass();
		BloomCompute();
		CompositePass();

		ResetDrawCommands();

		m_SceneCommandBuffer->End();
	}

	void SceneRenderer::TraceScene()
	{
		VK_CORE_PROFILE_FN("Submit-SceneRenderer");

		m_SceneCommandBuffer->Begin();

		RayTracePass();
		BloomCompute();
		CompositePass();

		m_SceneCommandBuffer->End();
	}

	void SceneRenderer::SetBuffersData(EditorCamera& camera)
	{
		int frameIndex = Renderer::GetCurrentFrameIndex();

		// Camera
		UBCamera cameraUB{};
		cameraUB.Projection = camera.GetProjectionMatrix();
		cameraUB.View = camera.GetViewMatrix();
		cameraUB.InverseProjection = glm::inverse(camera.GetProjectionMatrix());
		cameraUB.InverseView = glm::inverse(camera.GetViewMatrix());
		m_UBCamera[frameIndex]->WriteAndFlushBuffer(&cameraUB);

		// Point Light
		UBPointLights pointLightUB{};
		m_Scene->UpdatePointLightUB(pointLightUB);
		m_UBPointLight[frameIndex]->WriteAndFlushBuffer(&pointLightUB);

		UBSpotLights spotLightUB{};
		m_Scene->UpdateSpotLightUB(spotLightUB);
		m_UBSpotLight[frameIndex]->WriteAndFlushBuffer(&spotLightUB);
	}

	void SceneRenderer::RenderLights()
	{ 
		Renderer::Submit([this]
		{
			VkCommandBuffer bindCmd = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer)->RT_GetActiveCommandBuffer();
			auto vulkanLightPipeline = std::static_pointer_cast<VulkanPipeline>(m_LightPipeline);
			vulkanLightPipeline->Bind(bindCmd);

			// Binding Point Light Descriptor Set
			m_PointLightShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_LightPipeline);
		});

		// Point Lights
		for (auto pointLightPosition : m_PointLightPositions)
		{
			Renderer::Submit([this, pointLightPosition]
			{
				VK_CORE_PROFILE_FN("Render-PointLights");

				VkCommandBuffer drawCmd = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer)->RT_GetActiveCommandBuffer();

				auto vulkanLightPipeline = std::static_pointer_cast<VulkanPipeline>(m_LightPipeline);
				vulkanLightPipeline->SetPushConstants(drawCmd, (void*)&pointLightPosition, sizeof(glm::vec4));
				vkCmdDraw(drawCmd, 6, 1, 0, 0);
			});
		}

		Renderer::Submit([this] { m_SpotLightShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_LightPipeline); });

		// Spot Lights
		for (auto spotLightPosition : m_SpotLightPositions)
		{
			Renderer::Submit([this, spotLightPosition]
			{
				VK_CORE_PROFILE_FN("Render-SpotLights");

				VkCommandBuffer drawCmd = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer)->RT_GetActiveCommandBuffer();

				auto vulkanLightPipeline = std::static_pointer_cast<VulkanPipeline>(m_LightPipeline);
				vulkanLightPipeline->SetPushConstants(drawCmd, (void*)&spotLightPosition, sizeof(glm::vec4));
				vkCmdDraw(drawCmd, 6, 1, 0, 0);
			});
		}
	}

	void SceneRenderer::SubmitMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform)
	{
		VK_CORE_PROFILE();

		if (!mesh || !materialAsset)
			return;

		auto meshSource = mesh->GetMeshSource();
		auto& submeshData = meshSource->GetSubmeshes();
		uint64_t meshHandle = mesh->Handle;
		uint64_t materialHandle = materialAsset->Handle;

		if (meshSource->GetVertexCount() == 0)
			return;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			auto& transformBuffer = m_MeshTransformMap[meshKey].Transforms.emplace_back();

			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].LocalTransform;
			transformBuffer.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformBuffer.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformBuffer.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			auto& dc = m_MeshDrawList[meshKey];
			dc.MeshInstance = mesh;
			dc.MaterialInstance = materialAsset->GetMaterial();
			dc.SubmeshIndex = submeshIndex;
			dc.TransformBuffer = m_MeshTransformMap[meshKey].TransformBuffer;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::SubmitTransparentMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform)
	{

	}

	void SceneRenderer::SubmitRayTracedMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform)
	{
		auto meshSource = mesh->GetMeshSource();
		auto& submeshData = meshSource->GetSubmeshes();
		uint64_t meshHandle = mesh->Handle;
		uint64_t materialHandle = materialAsset->Handle;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			auto& transformBuffer = m_MeshTransformMap[meshKey].Transforms.emplace_back();

			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].LocalTransform;
			transformBuffer.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformBuffer.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformBuffer.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			auto& dc = m_MeshTraceList[meshKey];
			dc.MeshInstance = mesh;
			dc.MaterialInstance = materialAsset;
			dc.SubmeshIndex = submeshIndex;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::UpdateMeshInstanceData(std::shared_ptr<Mesh> mesh, std::shared_ptr<MaterialAsset> materialAsset)
	{
		uint64_t meshHandle = mesh->Handle;
		uint64_t materialHandle = materialAsset->Handle;

		for (uint32_t submeshIndex : mesh->GetSubmeshes())
		{
			MeshKey meshKey = { meshHandle, materialHandle, submeshIndex };
			m_MeshTransformMap.erase(meshKey);
			m_MeshDrawList.erase(meshKey);
		}
	}

	std::shared_ptr<Image2D> SceneRenderer::GetFinalPassImage(uint32_t index) const
	{
		auto framebuffer = std::dynamic_pointer_cast<VulkanFramebuffer>(m_SceneFramebuffer);
		return framebuffer->GetAttachment(true)[index];
	}
		
	void SceneRenderer::UpdateSkybox(const std::string& filepath)
	{
		// Obtain Cubemaps
		auto [filteredMap, irradianceMap] = VulkanRenderer::CreateEnviromentMap(filepath);
		m_CubemapTexture = filteredMap;
		m_PrefilteredTexture = filteredMap;
		m_IrradianceTexture = irradianceMap;

		// Update Materials
		m_GeometryMaterial->SetTexture(6, m_IrradianceTexture);
		m_GeometryMaterial->SetTexture(8, m_PrefilteredTexture);
		m_GeometryMaterial->PrepareShaderMaterial();

		m_SkyboxMaterial->SetTexture(1, m_CubemapTexture);
		m_SkyboxMaterial->PrepareShaderMaterial();

		ImGuiLayer::UpdateDescriptor(m_SkyboxTextureID, *std::dynamic_pointer_cast<VulkanTextureCube>(m_PrefilteredTexture));
	}

	void SceneRenderer::SetSkybox(const std::string& filepath)
	{
		s_Instance->UpdateSkybox(filepath);
	}

	void SceneRenderer::CompositePass()
	{
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Composite-Pass", DebugLabelColor::Red);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_CompositePipeline->GetSpecification().pRenderPass);

		Renderer::Submit([this]
		{
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);
			auto vulkanCompPipeline = std::static_pointer_cast<VulkanPipeline>(m_CompositePipeline);

			vulkanCompPipeline->SetPushConstants(vulkanCmdBuffer->RT_GetActiveCommandBuffer(), &m_SceneSettings, sizeof(SceneSettings));
		});

		Renderer::SubmitFullscreenQuad(m_SceneCommandBuffer, m_CompositePipeline, m_CompositeShaderMaterial);
		Renderer::EndRenderPass(m_SceneCommandBuffer, m_CompositePipeline->GetSpecification().pRenderPass);

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::GeometryPass()
	{
		m_Scene->OnUpdateGeometry(this);
		m_Scene->OnUpdateLights(m_PointLightPositions, m_SpotLightPositions);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().pRenderPass);

		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Skybox", DebugLabelColor::Orange);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		// Rendering Skybox
		Renderer::RenderSkybox(m_SceneCommandBuffer, m_SkyboxPipeline, m_SkyboxVBData, m_SkyboxMaterial, &m_SkyboxSettings);
		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		// Rendering Geometry
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Geometry-Pass", DebugLabelColor::Gold);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::Submit([this]
		{
			VkCommandBuffer bindCmd = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer)->RT_GetActiveCommandBuffer();
			auto vulkanGeometryPipeline = std::static_pointer_cast<VulkanPipeline>(m_GeometryPipeline);
			vulkanGeometryPipeline->Bind(bindCmd);

			// Binding Static Geometry Descriptor Sets
			m_GeometryMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_GeometryPipeline);
		});

		for (auto& [mk, dc] : m_MeshDrawList)
			Renderer::RenderMesh(m_SceneCommandBuffer, dc.MeshInstance, dc.MaterialInstance, dc.SubmeshIndex, m_GeometryPipeline, dc.TransformBuffer, m_MeshTransformMap[mk].Transforms, dc.InstanceCount);

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		// TODO: Render Transparent Meshes

		// Rendering Point Lights
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Lights", DebugLabelColor::Grey);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		RenderLights();

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);

		Renderer::EndRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().pRenderPass);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Copy-Image", DebugLabelColor::Pink);

		// Copying Image for Bloom
		int frameIndex = Renderer::GetCurrentFrameIndex();

		Renderer::CopyVulkanImage(m_SceneCommandBuffer,
			m_GeometryPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer->GetAttachment(true)[frameIndex],
			m_SceneRenderTextures[frameIndex]);

		Renderer::BlitVulkanImage(m_SceneCommandBuffer, m_SceneRenderTextures[frameIndex]);

		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::BloomCompute()
	{
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Bloom", DebugLabelColor::Blue);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::Submit([this]
		{
			VK_CORE_PROFILE_FN("SceneRenderer::BloomCompute");

			auto vulkanBloomPipeline = std::static_pointer_cast<VulkanComputePipeline>(m_BloomPipeline);
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);

			VkCommandBuffer dispatchCmd = vulkanCmdBuffer->RT_GetActiveCommandBuffer();
			vulkanBloomPipeline->Bind(dispatchCmd);

			// Prefilter
			m_LodAndMode.LOD = 0.0f;
			m_LodAndMode.Mode = 0.0f;

			m_BloomPrefilterShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

			const uint32_t mips = m_BloomTextures[0]->GetSpecification().MipLevels;
			glm::uvec2 bloomMipSize = m_BloomMipSize;

			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_BloomParams, sizeof(glm::vec2), sizeof(glm::vec2));
			vulkanBloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

			for (uint32_t i = 1; i < mips; ++i)
			{
				m_LodAndMode.LOD = float(i - 1);
				m_LodAndMode.Mode = 1.0f;

				int currentIdx = i - 1;

				m_BloomPingShaderMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
				bloomMipSize = m_BloomTextures[0]->GetMipSize(i);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

				m_LodAndMode.LOD = (float)i;
				
				m_BloomPongShaderMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);
			}

			// Upsample First
			// TODO: Could have to use VkImageSubresourceRange to set correct mip level
			m_LodAndMode.LOD = float(mips - 2);
			m_LodAndMode.Mode = 2.0f;

			m_BloomUpsampleFirstShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

			bloomMipSize = m_BloomTextures[2]->GetMipSize(mips - 1);

			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			vulkanBloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);

			// Upsample Final
			for (int i = mips - 2; i >= 0; --i)
			{
				m_LodAndMode.LOD = (float)i;
				m_LodAndMode.Mode = 3.0f;

				m_BloomUpsampleShaderMaterials[i]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

				bloomMipSize = m_BloomTextures[2]->GetMipSize(i);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, bloomMipSize.x / 16, bloomMipSize.y / 16, 1);
			}
		});

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::RayTracePass()
	{
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "RayTrace", DebugLabelColor::Aqua);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::TraceRays(m_SceneCommandBuffer, m_RayTracingPipeline, m_RayTracingMaterial, m_ViewportSize.x, m_ViewportSize.y);

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		// Copy Ray Tracing Output
		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Copy-RTOutput", DebugLabelColor::Green);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		int frameIndex = Renderer::GetCurrentFrameIndex();

		Renderer::CopyVulkanImage(m_SceneCommandBuffer,
			m_SceneRTOutputImages[frameIndex],
			m_SceneRenderTextures[frameIndex]);

		Renderer::BlitVulkanImage(m_SceneCommandBuffer, m_SceneRenderTextures[frameIndex]);

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::CreateCommandBuffers()
	{
		auto device = VulkanContext::GetCurrentDevice();
		m_SceneCommandBuffer = std::make_shared<VulkanRenderCommandBuffer>(device->GetCommandPool(), CommandBufferLevel::Primary, 5);
	}

	void SceneRenderer::ResetDrawCommands()
	{
		for (auto& [mk, dc] : m_MeshDrawList)
		{
			m_MeshTransformMap[mk].Transforms.clear();
			dc.InstanceCount = 0;
		}

		m_PointLightPositions.clear();
		m_SpotLightPositions.clear();
	}

}