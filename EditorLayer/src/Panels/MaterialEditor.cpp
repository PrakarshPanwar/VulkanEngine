#include <unordered_map>
#include <memory>
#include <filesystem>

#include "MaterialEditor.h"
#include "VulkanCore/Asset/AssetManager.h"
#include "Platform/Vulkan/VulkanMaterial.h"

#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>

namespace VulkanCore {

	static const std::filesystem::path g_AssetPath = "assets";

	std::unordered_map<uint64_t, std::shared_ptr<Material>> MaterialEditor::s_OpenedMaterials;

	enum class TextureType
	{
		None = 0,
		Diffuse,
		Normal,
		ARM,
		Displacement
	};

	namespace Utils {

		static void ResetTexturePopup(TextureType textureType, std::shared_ptr<VulkanMaterial> material)
		{
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				ImGui::OpenPopup("RemoveTexture");

			if (ImGui::BeginPopup("RemoveTexture"))
			{
				if (ImGui::MenuItem("Remove Texture"))
				{
					switch (textureType)
					{
					case TextureType::Diffuse:
						material->SetDiffuseTexture(Renderer::GetWhiteTexture(ImageFormat::RGBA8_SRGB));
						break;
					case TextureType::Normal:
						material->SetNormalTexture(Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM));
						break;
					case TextureType::ARM:
						material->SetARMTexture(Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM));
						break;
					case TextureType::Displacement:
						material->SetDisplacementTexture(Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM));
						break;
					default:
						break;
					}
				}

				ImGui::EndPopup();
			}
		}

	}

	MaterialEditor::MaterialEditor(std::shared_ptr<MaterialAsset> materialAsset)
		: m_MaterialAsset(materialAsset)
	{
	}

	MaterialEditor::~MaterialEditor()
	{
	}

	void MaterialEditor::OnImGuiRender()
	{
		if (m_OpenEditorWindow)
		{
			ImGui::Begin("Material Editor", &m_OpenEditorWindow);

			auto vulkanMaterial = std::dynamic_pointer_cast<VulkanMaterial>(m_MaterialAsset->GetMaterial());

			if (ImGui::Button("Save"))
				m_MaterialAsset->Serialize();
			ImGui::Separator();

			auto& materialData = vulkanMaterial->GetMaterialData();
			auto [diffuse, normal, arm, displacement] = vulkanMaterial->GetMaterialTextureIDs();

			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 4 });
			bool albedoNode = ImGui::TreeNodeEx("ALBEDO", treeNodeFlags);
			ImGui::PopStyleVar();

			if (albedoNode)
			{
				ImGui::Image((ImTextureID)diffuse, { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
				Utils::ResetTexturePopup(TextureType::Diffuse, vulkanMaterial);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path assetPath = g_AssetPath / path;

						std::shared_ptr<Texture2D> diffuseTex = AssetManager::GetAsset<Texture2D>(assetPath.string());
						vulkanMaterial->SetDiffuseTexture(diffuseTex);
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::ColorEdit4("Color", glm::value_ptr(materialData.Albedo), ImGuiColorEditFlags_NoInputs);
				if (ImGui::IsItemEdited())
					m_MaterialAsset->SetTransparent(materialData.Albedo.w < 1.0f);

				ImGui::DragFloat("Emission", &materialData.Emission, 0.01f, 0.0f, 10000.0f);

				ImGui::TreePop();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 4 });
			bool normalNode = ImGui::TreeNodeEx("NORMAL", treeNodeFlags);
			ImGui::PopStyleVar();

			if (normalNode)
			{
				ImGui::Image((ImTextureID)normal, { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
				Utils::ResetTexturePopup(TextureType::Normal, vulkanMaterial);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path assetPath = g_AssetPath / path;

						std::shared_ptr<Texture2D> normalTex = AssetManager::GetAsset<Texture2D>(assetPath.string());
						vulkanMaterial->SetNormalTexture(normalTex);
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				ImGui::Checkbox("Use", (bool*)&materialData.UseNormalMap);

				ImGui::TreePop();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 4 });
			bool armNode = ImGui::TreeNodeEx("AO/ROUGHNESS/METALLIC", treeNodeFlags);
			ImGui::PopStyleVar();

			if (armNode)
			{
				ImGui::Image((ImTextureID)arm, { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
				Utils::ResetTexturePopup(TextureType::ARM, vulkanMaterial);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path assetPath = g_AssetPath / path;

						std::shared_ptr<Texture2D> armTex = AssetManager::GetAsset<Texture2D>(assetPath.string());
						vulkanMaterial->SetARMTexture(armTex);
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::DragFloat("Roughness", &materialData.Roughness, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Metallic", &materialData.Metallic, 0.01f, 0.0f, 1.0f);

				ImGui::TreePop();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 4 });
			bool displacementNode = ImGui::TreeNodeEx("DISPLACEMENT", treeNodeFlags);
			ImGui::PopStyleVar();

			if (displacementNode)
			{
				ImGui::Image((ImTextureID)displacement, { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });
				Utils::ResetTexturePopup(TextureType::Displacement, vulkanMaterial);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path assetPath = g_AssetPath / path;

						std::shared_ptr<Texture2D> displacementTex = AssetManager::GetAsset<Texture2D>(assetPath.string());
						vulkanMaterial->SetDisplacementTexture(displacementTex);

						m_MaterialAsset->SetDisplacement(true);
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::TreePop();
			}

			ImGui::End();
		}
	}

}
