#include "vulkanpch.h"
#include "SceneRenderer.h"

#include "VulkanCore/Core/Core.h"
#include "VulkanCore/Core/Application.h"
#include "VulkanCore/Core/Timer.h"
#include "VulkanCore/Core/ImGuiLayer.h"
#include "VulkanCore/Events/Input.h"
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
#include "Platform/Vulkan/VulkanVertexBuffer.h"
#include "Platform/Vulkan/VulkanIndexBuffer.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace VulkanCore {

	namespace Utils {

		static uint32_t CalculateMipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::_Floor_of_log_2(std::max(width, height)) + 1;
		}

		static uint64_t GetBufferDeviceAddress(VkBuffer buffer, uint64_t offset = 0)
		{
			auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
			bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			bufferDeviceAddressInfo.buffer = buffer;
			
			uint64_t deviceAddress = vkGetBufferDeviceAddressKHR(vulkanDevice, &bufferDeviceAddressInfo);
			return deviceAddress + offset;
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

		m_Scene->OnUpdateRayTracedGeometry(this);

		std::vector<MeshBuffersAddress> meshBuffersData;
		std::vector<MaterialData> meshMaterialData;

		for (auto& [mk, tc] : m_MeshTraceList)
		{
			// Submit Mesh Buffers Data
			auto meshSource = tc.MeshInstance->GetMeshSource();

			auto vulkanMeshVB = std::static_pointer_cast<VulkanVertexBuffer>(meshSource->GetVertexBuffer());
			auto vulkanMeshIB = std::static_pointer_cast<VulkanIndexBuffer>(meshSource->GetIndexBuffer());

			auto& submeshData = meshSource->GetSubmeshes();
			auto& submesh = submeshData[tc.SubmeshIndex];

			auto& bufferData = meshBuffersData.emplace_back();
			bufferData.VertexBufferAddress = vulkanMeshVB->GetVulkanBufferDeviceAddress(submesh.BaseVertex * sizeof(Vertex));
			bufferData.IndexBufferAddress = vulkanMeshIB->GetVulkanBufferDeviceAddress(submesh.BaseIndex * sizeof(uint32_t));

			// Submit Mesh Material Data
			meshMaterialData.push_back(tc.MaterialInstance->GetMaterial()->GetMaterialData());

			m_DiffuseTextureArray.push_back(tc.MaterialInstance->GetMaterial()->GetDiffuseTexture());
			m_NormalTextureArray.push_back(tc.MaterialInstance->GetMaterial()->GetNormalTexture());
			m_ARMTextureArray.push_back(tc.MaterialInstance->GetMaterial()->GetARMTexture());

			// Submit Mesh Data to AS
			m_SceneAccelerationStructure->SubmitMeshDrawData(tc.MeshInstance, tc.MaterialInstance, m_MeshTransformMap[mk].Transforms, tc.SubmeshIndex, tc.InstanceCount);
		}

		m_SceneAccelerationStructure->BuildBottomLevelAccelerationStructures();
		m_SceneAccelerationStructure->BuildTopLevelAccelerationStructure();

		// Write Data in Storage Buffers
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto vulkanSBMeshBuffersData = std::make_shared<VulkanStorageBuffer>(sizeof(MeshBuffersAddress) * m_MeshTraceList.size());
			auto vulkanSBMeshMaterialData = std::make_shared<VulkanStorageBuffer>(sizeof(MaterialData) * m_MeshTraceList.size());

			m_SBMeshBuffersData.emplace_back(vulkanSBMeshBuffersData);
			m_SBMaterialDataBuffer.emplace_back(vulkanSBMeshMaterialData);

			// Set Debug Names
			VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(),
				VK_OBJECT_TYPE_BUFFER,
				std::format("Mesh Data Storage Buffer: {}", i),
				vulkanSBMeshBuffersData->GetDescriptorBufferInfo().buffer);

			VKUtils::SetDebugUtilsObjectName(device->GetVulkanDevice(),
				VK_OBJECT_TYPE_BUFFER,
				std::format("Mesh Material Storage Buffer: {}", i),
				vulkanSBMeshMaterialData->GetDescriptorBufferInfo().buffer);

			vulkanSBMeshBuffersData->WriteAndFlushBuffer(meshBuffersData.data(), 0);
			vulkanSBMeshMaterialData->WriteAndFlushBuffer(meshMaterialData.data(), 0);
		}

		CreateRayTraceMaterials();
	}

	void SceneRenderer::CreateRayTraceMaterials()
	{
		// Ray Tracing Material
		{
			m_RayTracingBaseMaterial = std::make_shared<VulkanMaterial>(m_RayTracingPipeline->GetShader(), "Ray Tracing Base Shader Material", 0);

			m_RayTracingBaseMaterial->SetAccelerationStructure(0, m_SceneAccelerationStructure);
			m_RayTracingBaseMaterial->SetImages(1, m_SceneRTOutputImages);
			m_RayTracingBaseMaterial->SetImages(2, m_SceneRTAccumulationImages);
			m_RayTracingBaseMaterial->SetBuffers(3, m_UBCamera);
			m_RayTracingBaseMaterial->SetBuffers(4, m_UBPointLight);
			//m_RayTracingBaseMaterial->SetBuffers(5, m_UBSpotLight);
			m_RayTracingBaseMaterial->SetBuffers(6, m_SBMeshBuffersData);
			m_RayTracingBaseMaterial->PrepareShaderMaterial();
		}

		// Ray Tracing PBR Material
		{
			m_RayTracingPBRMaterial = std::make_shared<VulkanMaterial>(m_RayTracingPipeline->GetShader(), "Ray Tracing PBR Material", 1);

			m_RayTracingPBRMaterial->SetTextureArray(0, m_DiffuseTextureArray);
			m_RayTracingPBRMaterial->SetTextureArray(1, m_NormalTextureArray);
			m_RayTracingPBRMaterial->SetTextureArray(2, m_ARMTextureArray);
			m_RayTracingPBRMaterial->SetBuffers(3, m_SBMaterialDataBuffer);
			m_RayTracingPBRMaterial->SetBuffers(4, m_UBRTMaterialData);
			m_RayTracingPBRMaterial->PrepareShaderMaterial();
		}

		// Ray Tracing Skybox Material
		{
			m_RayTracingSkyboxMaterial = std::make_shared<VulkanMaterial>(m_RayTracingPipeline->GetShader(), "Ray Tracing Skybox Material", 2);

			m_RayTracingSkyboxMaterial->SetTexture(0, m_HDRTexture);
			m_RayTracingSkyboxMaterial->SetTexture(1, m_PDFTexture);
			m_RayTracingSkyboxMaterial->SetTexture(2, m_CDFTexture);
			m_RayTracingSkyboxMaterial->SetBuffers(3, m_UBSkyboxSettings);
			m_RayTracingSkyboxMaterial->PrepareShaderMaterial();
		}
	}

	void SceneRenderer::CreatePipelines()
	{
		VertexBufferLayout vertexLayout = { {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float2, "a_TexCoord" } },
			16 // Alignment
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
			geomFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::DEPTH32F };
			geomFramebufferSpec.ReadDepthTexture = true;
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

		// Geometry/Light Selection Pipeline(Editor Only)
		{
			FramebufferSpecification geomSelectFramebufferSpec;
			geomSelectFramebufferSpec.Width = 1920;
			geomSelectFramebufferSpec.Height = 1080;
			geomSelectFramebufferSpec.Attachments = { ImageFormat::R32I, ImageFormat::DEPTH24STENCIL8 };
			geomSelectFramebufferSpec.Transfer = true;
			geomSelectFramebufferSpec.Samples = 1;
			memset(&geomSelectFramebufferSpec.ClearColor, -1, sizeof(glm::vec4));

			RenderPassSpecification geomSelectRenderPassSpec;
			geomSelectRenderPassSpec.TargetFramebuffer = std::make_shared<VulkanFramebuffer>(geomSelectFramebufferSpec);

			PipelineSpecification geomSelectPipelineSpec;
			geomSelectPipelineSpec.DebugName = "Geometry Select Pipeline";
			geomSelectPipelineSpec.pShader = Renderer::GetShader("CoreEditor");
			geomSelectPipelineSpec.pRenderPass = std::make_shared<VulkanRenderPass>(geomSelectRenderPassSpec);
			geomSelectPipelineSpec.Layout = vertexLayout;
			geomSelectPipelineSpec.InstanceLayout = {
				{ ShaderDataType::Float4, "a_MRow0" },
				{ ShaderDataType::Float4, "a_MRow1" },
				{ ShaderDataType::Float4, "a_MRow2" },
				{ ShaderDataType::Int, "a_EntityID" }
			};

			m_GeometrySelectPipeline = std::make_shared<VulkanPipeline>(geomSelectPipelineSpec);

			PipelineSpecification lightSelectPipelineSpec;
			lightSelectPipelineSpec.DebugName = "Light Select Pipeline";
			lightSelectPipelineSpec.pShader = Renderer::GetShader("LightEditor");
			lightSelectPipelineSpec.pRenderPass = geomSelectPipelineSpec.pRenderPass;

			m_LightSelectPipeline = std::make_shared<VulkanPipeline>(lightSelectPipelineSpec);
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
			skyboxPipelineSpec.pRenderPass = m_GeometryPipeline->GetSpecification().pRenderPass;
			skyboxPipelineSpec.DepthCompareOp = CompareOp::LessOrEqual;

			m_SkyboxPipeline = std::make_shared<VulkanPipeline>(skyboxPipelineSpec);
		}

		// Bloom Pipeline
		{
			m_BloomPipeline = std::make_shared<VulkanComputePipeline>(Renderer::GetShader("Bloom"), "Bloom Pipeline");
		}

		// Ray Tracing Pipeline
		{
			std::vector<HitShaderInfo> hitShaderInfos = {
				{ "CoreRT.rchit", "CoreRT.rahit" }
			};

			std::vector<std::string> missPaths = {
				"CoreRT.rmiss", "Shadow.rmiss"
			};

			m_RayTracingSBT = std::make_shared<VulkanShaderBindingTable>("CoreRT.rgen", hitShaderInfos, missPaths);
			m_RayTracingPipeline = std::make_shared<VulkanRayTracingPipeline>(m_RayTracingSBT, "Ray Tracing Pipeline");
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

		// Geometry Select Material
		{
			m_GeometrySelectMaterial = std::make_shared<VulkanMaterial>(m_GeometrySelectPipeline->GetSpecification().pShader, "Geometry Select Shader Material");

			m_GeometrySelectMaterial->SetBuffers(0, m_UBCamera);
			m_GeometrySelectMaterial->PrepareShaderMaterial();
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

			m_LightSelectMaterial = std::make_shared<VulkanMaterial>(m_LightSelectPipeline->GetSpecification().pShader, "Light Select Shader Material");

			m_LightSelectMaterial->SetBuffers(0, m_UBCamera);
			m_LightSelectMaterial->PrepareShaderMaterial();
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
			m_CompositeShaderMaterial->SetBuffers(0, m_UBCamera);
			m_CompositeShaderMaterial->SetImages(1, m_RayTraced ? m_SceneRTOutputImages : geomFB->GetAttachment(true));
			m_CompositeShaderMaterial->SetImages(2, geomFB->GetDepthAttachment(true));
			m_CompositeShaderMaterial->SetImage(3, m_BloomTextures[2]);
			m_CompositeShaderMaterial->SetTexture(4, m_BloomDirtTexture);
			m_CompositeShaderMaterial->PrepareShaderMaterial();
		}

		// Skybox Material
		{
			m_SkyboxMaterial = std::make_shared<VulkanMaterial>(m_SkyboxPipeline->GetSpecification().pShader, "Skybox Shader Material");

			m_SkyboxMaterial->SetBuffers(0, m_UBCamera);
			m_SkyboxMaterial->SetTexture(1, m_PrefilteredTexture);
			m_SkyboxMaterial->PrepareShaderMaterial();
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

			m_CompositeShaderMaterial->SetBuffers(0, m_UBCamera);
			m_CompositeShaderMaterial->SetImages(1, m_RayTraced ? m_SceneRTOutputImages : geomFB->GetAttachment(true));
			m_CompositeShaderMaterial->SetImages(2, geomFB->GetDepthAttachment(true));
			m_CompositeShaderMaterial->SetImage(3, m_BloomTextures[2]);
			m_CompositeShaderMaterial->SetTexture(4, m_BloomDirtTexture);
			m_CompositeShaderMaterial->PrepareShaderMaterial();
		}

		// Skybox Material
		{
			m_SkyboxMaterial->SetBuffers(0, m_UBCamera);
			m_SkyboxMaterial->SetTexture(1, m_PrefilteredTexture);
			m_SkyboxMaterial->PrepareShaderMaterial();
		}

		if (m_RayTraced)
		{
			// Ray Tracing Base Material
			m_RayTracingBaseMaterial->SetImages(1, m_SceneRTOutputImages);
			m_RayTracingBaseMaterial->SetImages(2, m_SceneRTAccumulationImages);
			m_RayTracingBaseMaterial->PrepareShaderMaterial();
		}
	}

	void SceneRenderer::RecreatePipelines()
	{
		m_GeometryPipeline->ReloadPipeline();
		m_LightPipeline->ReloadPipeline();
		m_CompositePipeline->ReloadPipeline();
		m_SkyboxPipeline->ReloadPipeline();
		m_BloomPipeline->ReloadPipeline();

		if (m_RayTraced)
			m_RayTracingPipeline->ReloadPipeline();
	}

	void SceneRenderer::CreateResources()
	{
		auto device = VulkanContext::GetCurrentDevice();

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Renderer::WaitAndExecute();

		m_UBCamera.reserve(framesInFlight);
		m_UBPointLight.reserve(framesInFlight);
		m_UBSpotLight.reserve(framesInFlight);
		m_UBSkyboxSettings.reserve(framesInFlight);
		m_UBRTMaterialData.reserve(framesInFlight);
		m_ImageBuffer.reserve(framesInFlight);
		m_SBMeshBuffersData.reserve(framesInFlight);
		m_SBMaterialDataBuffer.reserve(framesInFlight);

		// Uniform Buffers
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			m_UBCamera.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBCamera)));
			m_UBPointLight.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBPointLights)));
			m_UBSpotLight.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(UBSpotLights)));
			m_UBSkyboxSettings.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(SkyboxSettings)));
			m_UBRTMaterialData.emplace_back(std::make_shared<VulkanUniformBuffer>(sizeof(RTMaterialData)));
			m_ImageBuffer.emplace_back(std::make_shared<VulkanIndexBuffer>(m_ViewportSize.x * m_ViewportSize.y * sizeof(int)));
		}

		m_BloomMipSize = (glm::uvec2(m_ViewportSize.x, m_ViewportSize.y) + 1u) / 2u;
		m_BloomMipSize += 16u - m_BloomMipSize % 16u;

		m_BloomTextures.reserve(framesInFlight);
		m_SceneRenderTextures.reserve(framesInFlight);

		VkCommandBuffer barrierCmd = device->GetCommandBuffer();

		// Bloom Compute Textures
		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			ImageSpecification bloomRTSpec = {};
			bloomRTSpec.DebugName = std::format("Bloom Compute Texture {}", i);
			bloomRTSpec.Width = m_BloomMipSize.x;
			bloomRTSpec.Height = m_BloomMipSize.y;
			bloomRTSpec.Format = ImageFormat::RGBA32F;
			bloomRTSpec.Usage = ImageUsage::Storage;
			bloomRTSpec.MipLevels = Utils::CalculateMipCount(m_BloomMipSize.x, m_BloomMipSize.y) - 2;

			auto BloomTexture = std::static_pointer_cast<VulkanImage>(m_BloomTextures.emplace_back(std::make_shared<VulkanImage>(bloomRTSpec)));
			BloomTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, BloomTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, BloomTexture->GetSpecification().MipLevels, 0, 1 });
		}

		// Scene Render Textures
		ImageSpecification sceneRTSpec = {};
		sceneRTSpec.DebugName = "Scene Render Texture";
		sceneRTSpec.Width = m_ViewportSize.x;
		sceneRTSpec.Height = m_ViewportSize.y;
		sceneRTSpec.Format = ImageFormat::RGBA32F;
		sceneRTSpec.Usage = ImageUsage::Texture;
		sceneRTSpec.MipLevels = Utils::CalculateMipCount(m_ViewportSize.x, m_ViewportSize.y);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto SceneTexture = std::static_pointer_cast<VulkanImage>(m_SceneRenderTextures.emplace_back(std::make_shared<VulkanImage>(sceneRTSpec)));
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

		ImageSpecification sceneRTAccumulateSpec = {};
		sceneRTAccumulateSpec.DebugName = "Scene Accumulate Output";
		sceneRTAccumulateSpec.Width = m_ViewportSize.x;
		sceneRTAccumulateSpec.Height = m_ViewportSize.y;
		sceneRTAccumulateSpec.Format = ImageFormat::RGBA32F;
		sceneRTAccumulateSpec.Usage = ImageUsage::Storage;

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			auto SceneRTAccumulateTexture = std::static_pointer_cast<VulkanImage>(m_SceneRTAccumulationImages.emplace_back(std::make_shared<VulkanImage>(sceneRTAccumulateSpec)));
			SceneRTAccumulateTexture->Invalidate();

			Utils::InsertImageMemoryBarrier(barrierCmd, SceneRTAccumulateTexture->GetVulkanImageInfo().Image,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneRTAccumulateTexture->GetSpecification().MipLevels, 0, 1 });
		}

		device->FlushCommandBuffer(barrierCmd);

		m_SceneImages.resize(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			std::shared_ptr<VulkanImage> finalPassImage = std::dynamic_pointer_cast<VulkanImage>(GetFinalPassImage(i));
			m_SceneImages[i] = ImGuiLayer::AddTexture(*finalPassImage);
		}

		m_BloomDirtTexture = AssetManager::GetAsset<Texture2D>("assets/textures/LensDirt.png");

		auto blackTextureCube = Renderer::GetBlackTextureCube(ImageFormat::RGBA8_UNORM);
		m_PrefilteredTexture = blackTextureCube;
		m_IrradianceTexture = blackTextureCube;

		auto whiteTexture = Renderer::GetWhiteTexture();
		m_HDRTexture = whiteTexture;
		m_PDFTexture = whiteTexture;
		m_CDFTexture = whiteTexture;

		m_SkyboxTextureID = ImGuiLayer::AddTexture(*std::dynamic_pointer_cast<VulkanTextureCube>(m_PrefilteredTexture));

		m_BRDFTexture = Renderer::CreateBRDFTexture();
		m_PointLightTextureIcon = TextureImporter::LoadTexture2D("../EditorLayer/Resources/Icons/PointLightIcon.png");
		m_SpotLightTextureIcon = TextureImporter::LoadTexture2D("../EditorLayer/Resources/Icons/SpotLightIcon.png");

		m_SceneAccelerationStructure = std::make_shared<VulkanAccelerationStructure>();
	}

	void SceneRenderer::Release()
	{
	}

	void SceneRenderer::RecreateScene()
	{
		auto device = VulkanContext::GetCurrentDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;

		VK_CORE_INFO("Scene Resized!");
		m_GeometryPipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
		m_GeometrySelectPipeline->GetSpecification().pRenderPass->RecreateFramebuffers(m_ViewportSize.x, m_ViewportSize.y);
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

			for (auto& SceneRTAccumulateTexture : m_SceneRTAccumulationImages)
			{
				auto vulkanRTAccumulateImage = std::static_pointer_cast<VulkanImage>(SceneRTAccumulateTexture);
				vulkanRTAccumulateImage->Resize(m_ViewportSize.x, m_ViewportSize.y);

				Utils::InsertImageMemoryBarrier(barrierCmd, vulkanRTAccumulateImage->GetVulkanImageInfo().Image,
					VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
					VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, SceneRTAccumulateTexture->GetSpecification().MipLevels, 0, 1 });
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
			ImGui::Checkbox("Fog", (bool*)&m_SceneSettings.Fog);

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene Renderer Stats##GPUPerf", treeFlags))
		{
			auto vulkanCmdBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);

			if (m_RayTraced)
			{
				ImGui::Text("RayTrace Pass: %lluns", vulkanCmdBuffer->GetQueryTime(0));
				ImGui::Text("Composite Pass: %lluns", vulkanCmdBuffer->GetQueryTime(3));
				ImGui::Text("Bloom Compute Pass: %lluns", vulkanCmdBuffer->GetQueryTime(2));
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
			// Ray Tracing
			{
				ImGui::SetNextItemAllowOverlap();
				ImGui::Selectable("RayTrace", false, ImGuiSelectableFlags_AllowOverlap, ImVec2{0.0f, 24.5f});

				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
				ImGui::PushID(buttonID);
				if (ImGui::Button("Reload", ImVec2{ 0.0f, 24.5f }))
				{
					m_RayTracingSBT->GetShader()->Reload();
					m_RayTracingPipeline->ReloadPipeline();
				}

				ImGui::PopID();
				buttonID++;
			}

			ImGui::Separator();

			for (auto&& [name, shader] : shaderMap)
			{
				ImGui::SetNextItemAllowOverlap();
				ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_AllowOverlap, ImVec2{ 0.0f, 24.5f });

				ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
				ImGui::PushID(buttonID);
				if (ImGui::Button("Reload", ImVec2{ 0.0f, 24.5f }))
				{
					shader->Reload();
					RecreatePipelines();
				}

				ImGui::PopID();
				buttonID++;
			}

			ImGui::TreePop();
		}

		if (m_RayTraced)
		{
			if (ImGui::TreeNodeEx("Scene Ray Tracer Settings", treeFlags))
			{
				uint32_t DragMin = 1;
				uint32_t DragMax = 16;
				ImGui::SliderScalar("Sample Count", ImGuiDataType_U32, &m_RTSettings.SampleCount, &DragMin, &DragMax);
				ImGui::SliderScalar("Bounces", ImGuiDataType_U32, &m_RTSettings.Bounces, &DragMin, &DragMax);

				ImGui::ColorEdit3("Extinction", glm::value_ptr(m_RTMaterialData.Extinction_AtDistance));
				ImGui::DragFloat("At Distance", &m_RTMaterialData.Extinction_AtDistance.w, 0.01f, 0.001f, 1.0f);
				ImGui::DragFloat("Transmission", &m_RTMaterialData.Transmission, 0.01f, 0.001f, 1.0f);
				ImGui::DragFloat("Specular Tint", &m_RTMaterialData.SpecularTint, 0.01f, 0.001f, 1.0f);
				ImGui::DragFloat("IOR", &m_RTMaterialData.IOR, 0.01f, 0.001f, 0.999f);
				ImGui::DragFloat("Sheen", &m_RTMaterialData.Sheen, 0.01f, 0.001f, 1.0f);
				ImGui::DragFloat("Sheen Tint", &m_RTMaterialData.SheenTint, 0.01f, 0.001f, 1.0f);
				ImGui::DragFloat("Clearcoat", &m_RTMaterialData.Clearcoat, 0.01f, 0.001f, 1.0f);
				ImGui::DragFloat("Clearcoat Gloss", &m_RTMaterialData.ClearcoatGloss, 0.001f, 0.01f, 1.0f);
				ImGui::DragFloat("Anisotropy", &m_RTMaterialData.Anisotropy, 0.01f, 0.001f, 1.0f);
				ImGui::DragFloat("Subsurface", &m_RTMaterialData.Subsurface, 0.01f, 0.001f, 1.0f);

				ImGui::Checkbox("Accumulate", &m_Accumulate);
				ImGui::Text("Accumulate Frame Count: %u", m_RTSettings.AccumulateFrameIndex);

				ImGui::TreePop();
			}
		}
		else if (ImGui::TreeNodeEx("Scene Draw Stats", treeFlags))
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

		m_MeshDrawList.clear();
		m_MeshTransformMap.clear();
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

		int frameIndex = Renderer::GetCurrentFrameIndex();

		// Camera
		UBCamera cameraUB{};
		cameraUB.Projection = m_SceneEditorData.CameraData.GetProjectionMatrix();
		cameraUB.View = m_SceneEditorData.CameraData.GetViewMatrix();
		cameraUB.InverseView = glm::inverse(m_SceneEditorData.CameraData.GetViewMatrix());
		cameraUB.InverseProjection = glm::inverse(m_SceneEditorData.CameraData.GetProjectionMatrix());

		glm::vec2 nearFarClip = m_SceneEditorData.CameraData.GetNearFarClip();
		cameraUB.DepthUnpackConsts.x = (nearFarClip.y * nearFarClip.x) / (nearFarClip.y - nearFarClip.x);
		cameraUB.DepthUnpackConsts.y = (nearFarClip.y + nearFarClip.x) / (nearFarClip.y - nearFarClip.x);

		m_UBCamera[frameIndex]->WriteAndFlushBuffer(&cameraUB);

		// Lights
		UBPointLights pointLightUB{};
		m_Scene->UpdatePointLightUB(pointLightUB);
		m_UBPointLight[frameIndex]->WriteAndFlushBuffer(&pointLightUB);

		UBSpotLights spotLightUB{};
		m_Scene->UpdateSpotLightUB(spotLightUB);
		m_UBSpotLight[frameIndex]->WriteAndFlushBuffer(&spotLightUB);

		// Skybox Data
		m_UBSkyboxSettings[frameIndex]->WriteAndFlushBuffer(&m_SkyboxSettings);

		m_SceneCommandBuffer->Begin();

		GeometryPass();
		BloomCompute();
		CompositePass();

		if (m_SceneEditorData.ViewportHovered)
			SelectionPass();

		ResetDrawCommands();

		m_SceneCommandBuffer->End();
	}

	void SceneRenderer::TraceScene()
	{
		VK_CORE_PROFILE_FN("Submit-SceneRenderer");

		int frameIndex = Renderer::GetCurrentFrameIndex();

		// Camera
		UBCamera cameraUB{};
		cameraUB.Projection = m_SceneEditorData.CameraData.GetProjectionMatrix();
		cameraUB.View = m_SceneEditorData.CameraData.GetViewMatrix();
		cameraUB.InverseProjection = glm::inverse(m_SceneEditorData.CameraData.GetProjectionMatrix());
		cameraUB.InverseView = glm::inverse(m_SceneEditorData.CameraData.GetViewMatrix());

		glm::vec2 nearFarClip = m_SceneEditorData.CameraData.GetNearFarClip();
		cameraUB.DepthUnpackConsts.x = (nearFarClip.y * nearFarClip.x) / (nearFarClip.y - nearFarClip.x);
		cameraUB.DepthUnpackConsts.y = (nearFarClip.y + nearFarClip.x) / (nearFarClip.y - nearFarClip.x);

		m_UBCamera[frameIndex]->WriteAndFlushBuffer(&cameraUB);

		// Lights
		UBPointLights pointLightUB{};
		m_Scene->UpdatePointLightUB(pointLightUB);
		m_UBPointLight[frameIndex]->WriteAndFlushBuffer(&pointLightUB);

		UBSpotLights spotLightUB{};
		m_Scene->UpdateSpotLightUB(spotLightUB);
		m_UBSpotLight[frameIndex]->WriteAndFlushBuffer(&spotLightUB);

		// Skybox Data
		m_UBSkyboxSettings[frameIndex]->WriteAndFlushBuffer(&m_SkyboxSettings);

		// Ray Tracing Material Data
		m_UBRTMaterialData[frameIndex]->WriteAndFlushBuffer(&m_RTMaterialData);

		m_SceneCommandBuffer->Begin();

		RayTracePass();
		BloomCompute();
		CompositePass();

		if (m_SceneEditorData.ViewportHovered)
			SelectionPass();

		ResetTraceCommands();

		m_SceneCommandBuffer->End();
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

	void SceneRenderer::SubmitSelectedMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform, uint32_t entityID)
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
			auto& transformBuffer = m_SelectedMeshTransformMap[meshKey].Transforms.emplace_back();

			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].LocalTransform;
			transformBuffer.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformBuffer.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformBuffer.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };
			transformBuffer.EntityID = entityID;

			auto& dc = m_SelectedMeshDrawList[meshKey];
			dc.MeshInstance = mesh;
			dc.SubmeshIndex = submeshIndex;
			dc.TransformBuffer = m_SelectedMeshTransformMap[meshKey].TransformBuffer;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::SubmitTransparentMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform)
	{

	}

	void SceneRenderer::SubmitRayTracedMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<MaterialAsset>& materialAsset, const glm::mat4& transform)
	{
		VK_CORE_PROFILE();

		if (!mesh || !materialAsset)
			return;

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
		auto [filteredMap, irradianceMap] = Renderer::CreateEnviromentMap(filepath);
		m_PrefilteredTexture = filteredMap;
		m_IrradianceTexture = irradianceMap;

		// Update Materials
		m_GeometryMaterial->SetTexture(6, m_IrradianceTexture);
		m_GeometryMaterial->SetTexture(8, m_PrefilteredTexture);
		m_GeometryMaterial->PrepareShaderMaterial();

		m_SkyboxMaterial->SetTexture(1, m_PrefilteredTexture);
		m_SkyboxMaterial->PrepareShaderMaterial();

		if (m_RayTraced)
		{
			m_HDRTexture = AssetManager::GetAsset<Texture2D>(filepath);
			auto [PDFTexture, CDFTexture] = Renderer::CreatePDFCDFTextures(m_HDRTexture);
			m_PDFTexture = PDFTexture;
			m_CDFTexture = CDFTexture;
		}

		ImGuiLayer::UpdateDescriptor(m_SkyboxTextureID, *std::dynamic_pointer_cast<VulkanTextureCube>(m_PrefilteredTexture));
	}

	void SceneRenderer::UpdateAccelerationStructure()
	{
		m_UpdateTLAS = true;
		m_RTSettings.AccumulateFrameIndex = 1;
	}

	void SceneRenderer::SetSkybox(const std::string& filepath)
	{
		s_Instance->UpdateSkybox(filepath);
	}

	void SceneRenderer::ResetAccumulationFrameIndex()
	{
		Renderer::Submit([this]
		{
			m_RTSettings.AccumulateFrameIndex = 1;
		});
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
		m_Scene->OnUpdateLights(m_PointLightPositions, m_SpotLightPositions, m_LightHandles);

		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_GeometryPipeline->GetSpecification().pRenderPass);

		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Skybox", DebugLabelColor::Orange);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		// Rendering Skybox
		Renderer::RenderSkybox(m_SceneCommandBuffer, m_SkyboxPipeline, m_SkyboxMaterial, &m_SkyboxSettings);
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

	void SceneRenderer::SelectionPass()
	{
		m_Scene->OnSelectGeometry(this);

		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "Selection-Pass", DebugLabelColor::Green);
		Renderer::BeginRenderPass(m_SceneCommandBuffer, m_GeometrySelectPipeline->GetSpecification().pRenderPass);

		for (auto& [mk, dc] : m_SelectedMeshDrawList)
			Renderer::RenderSelectedMesh(m_SceneCommandBuffer, dc.MeshInstance, m_GeometrySelectMaterial, dc.SubmeshIndex, m_GeometrySelectPipeline, dc.TransformBuffer, m_SelectedMeshTransformMap[mk].Transforms, dc.InstanceCount);

		// Lights
		{
			auto commandBuffer = std::static_pointer_cast<VulkanRenderCommandBuffer>(m_SceneCommandBuffer);
			auto lightPipeline = std::static_pointer_cast<VulkanPipeline>(m_LightSelectPipeline);

			Renderer::Submit([this, commandBuffer, lightPipeline]
			{
				VkCommandBuffer bindCmd = commandBuffer->RT_GetActiveCommandBuffer();
				lightPipeline->Bind(bindCmd);

				// Binding Point Light Descriptor Set
				m_LightSelectMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_LightSelectPipeline);
			});

			int index = 0;

			// Point Lights
			for (auto pointLightPosition : m_PointLightPositions)
			{
				uint32_t lightEntity = m_LightHandles[index++];

				Renderer::Submit([this, commandBuffer, lightPipeline, pointLightPosition, lightEntity]
				{
					VkCommandBuffer drawCmd = commandBuffer->RT_GetActiveCommandBuffer();

					lightPipeline->SetPushConstants(drawCmd, (void*)&pointLightPosition, sizeof(glm::vec4) + sizeof(int));
					vkCmdDraw(drawCmd, 6, 1, 0, 0);
				});
			}

			// Spot Lights
			for (auto spotLightPosition : m_SpotLightPositions)
			{
				uint32_t lightEntity = m_LightHandles[index++];

				Renderer::Submit([this, commandBuffer, lightPipeline, spotLightPosition, lightEntity]
				{
					VkCommandBuffer drawCmd = commandBuffer->RT_GetActiveCommandBuffer();

					lightPipeline->SetPushConstants(drawCmd, (void*)&spotLightPosition, sizeof(glm::vec4) + sizeof(int));
					vkCmdDraw(drawCmd, 6, 1, 0, 0);
				});
			}
		}

		Renderer::EndRenderPass(m_SceneCommandBuffer, m_GeometrySelectPipeline->GetSpecification().pRenderPass);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);

		Renderer::Submit([this]
		{
			int frameIndex = Renderer::RT_GetCurrentFrameIndex();
			auto frameBuffer = std::static_pointer_cast<VulkanFramebuffer>(m_GeometrySelectPipeline->GetSpecification().pRenderPass->GetSpecification().TargetFramebuffer);

			void* pixelData = frameBuffer->ReadPixel(m_SceneCommandBuffer, m_ImageBuffer[frameIndex], 0, m_SceneEditorData.ViewportMousePos.x, m_SceneEditorData.ViewportMousePos.y);
			m_HoveredEntity = *(int*)pixelData;
		});
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

			const uint32_t mips = m_BloomTextures[0]->GetSpecification().MipLevels;

			m_BloomPrefilterShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
			glm::uvec2 workGroups = glm::ceil((glm::vec2)m_BloomMipSize / 16.0f);

			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_BloomParams, sizeof(glm::vec2), sizeof(glm::vec2));
			vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);

			for (uint32_t i = 1; i < mips; ++i)
			{
				m_LodAndMode.LOD = float(i - 1);
				m_LodAndMode.Mode = 1.0f;

				int currentIdx = i - 1;

				m_BloomPingShaderMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
				workGroups = glm::ceil((glm::vec2)m_BloomTextures[0]->GetMipSize(i) / 16.0f);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);

				m_LodAndMode.LOD = (float)i;
				
				m_BloomPongShaderMaterials[currentIdx]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);
			}

			// Upsample First
			// TODO: Could have to use VkImageSubresourceRange to set correct mip level
			m_LodAndMode.LOD = float(mips - 2);
			m_LodAndMode.Mode = 2.0f;

			m_BloomUpsampleFirstShaderMaterial->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
			workGroups = glm::ceil((glm::vec2)m_BloomTextures[2]->GetMipSize(mips - 1) / 16.0f);

			vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
			vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);

			// Upsample Final
			for (int i = mips - 2; i >= 0; --i)
			{
				m_LodAndMode.LOD = (float)i;
				m_LodAndMode.Mode = 3.0f;

				m_BloomUpsampleShaderMaterials[i]->RT_BindMaterial(m_SceneCommandBuffer, m_BloomPipeline);
				workGroups = glm::ceil((glm::vec2)m_BloomTextures[2]->GetMipSize(i) / 16.0f);

				vulkanBloomPipeline->SetPushConstants(dispatchCmd, &m_LodAndMode, sizeof(glm::vec2));
				vulkanBloomPipeline->Dispatch(dispatchCmd, workGroups.x, workGroups.y, 1);
			}
		});

		Renderer::EndTimestampsQuery(m_SceneCommandBuffer);
		Renderer::EndGPUPerfMarker(m_SceneCommandBuffer);
	}

	void SceneRenderer::RayTracePass()
	{
		m_Scene->OnUpdateRayTracedGeometry(this);

		if (m_UpdateTLAS)
		{
			// Update TLAS Instance Data
			for (auto&& [mk, tc] : m_MeshTraceList)
				m_SceneAccelerationStructure->UpdateInstancesData(tc.MeshInstance, tc.MaterialInstance, m_MeshTransformMap[mk].Transforms, tc.SubmeshIndex);
		
			m_SceneAccelerationStructure->UpdateTopLevelAccelerationStructure(m_SceneCommandBuffer);
			m_UpdateTLAS = false;
		}

		Renderer::BeginGPUPerfMarker(m_SceneCommandBuffer, "RayTrace", DebugLabelColor::Aqua);
		Renderer::BeginTimestampsQuery(m_SceneCommandBuffer);

		Renderer::TraceRays(m_SceneCommandBuffer, m_RayTracingPipeline, m_RayTracingSBT,
			{ m_RayTracingBaseMaterial, m_RayTracingPBRMaterial, m_RayTracingSkyboxMaterial },
			m_ViewportSize.x, m_ViewportSize.y,
			&m_RTSettings, (uint32_t)sizeof(RTSettings));

		Renderer::Submit([this]
		{
			// Ray Tracing Data
			m_RTSettings.RandomSeed = 1;
			if (m_Accumulate)
			{
				m_RTSettings.AccumulateFrameIndex++;
				m_RTSettings.AccumulateFrameIndex = glm::clamp(m_RTSettings.AccumulateFrameIndex, 1u, m_RTSettings.SampleCount * m_ViewportSize.x * m_ViewportSize.y);
			}
			else
				m_RTSettings.AccumulateFrameIndex = 1;
		});

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

		for (auto& [mk, dc] : m_SelectedMeshDrawList)
		{
			m_SelectedMeshTransformMap[mk].Transforms.clear();
			dc.InstanceCount = 0;
		}

		m_PointLightPositions.clear();
		m_SpotLightPositions.clear();
		m_LightHandles.clear();
	}

	void SceneRenderer::ResetTraceCommands()
	{
		for (auto& [mk, tc] : m_MeshTraceList)
		{
			m_MeshTransformMap[mk].Transforms.clear();
			tc.InstanceCount = 0;
		}

		for (auto& [mk, dc] : m_SelectedMeshDrawList)
		{
			m_SelectedMeshTransformMap[mk].Transforms.clear();
			dc.InstanceCount = 0;
		}

		m_PointLightPositions.clear();
		m_SpotLightPositions.clear();
		m_LightHandles.clear();
	}

}