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
			auto [diffuse, normal, arm] = vulkanMaterial->GetMaterialTextureIDs();

			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 4 });
			bool albedoNode = ImGui::TreeNodeEx("ALBEDO", treeNodeFlags);
			ImGui::PopStyleVar();

			if (albedoNode)
			{
				ImGui::Image((ImTextureID)diffuse, { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });

				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
					ImGui::OpenPopup("RemoveTexture");

				if (ImGui::BeginPopup("RemoveTexture"))
				{
					if (ImGui::MenuItem("Remove Texture"))
					{
						auto whiteTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_SRGB);
						vulkanMaterial->SetDiffuseTexture(whiteTexture);
					}

					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path scenePath = g_AssetPath / path;

						std::shared_ptr<Texture2D> diffuseTex = AssetManager::GetAsset<Texture2D>(scenePath.string());
						vulkanMaterial->SetDiffuseTexture(diffuseTex);
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::ColorEdit3("Color", glm::value_ptr(materialData.Albedo), ImGuiColorEditFlags_NoInputs);
				ImGui::DragFloat("Emission", &materialData.Albedo.w, 0.01f, 0.0f, 10000.0f);

				ImGui::TreePop();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 4 });
			bool normalNode = ImGui::TreeNodeEx("NORMAL", treeNodeFlags);
			ImGui::PopStyleVar();

			if (normalNode)
			{
				ImGui::Image((ImTextureID)normal, { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });

				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
					ImGui::OpenPopup("RemoveTexture");

				if (ImGui::BeginPopup("RemoveTexture"))
				{
					if (ImGui::MenuItem("Remove Texture"))
					{
						auto whiteTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM);
						vulkanMaterial->SetNormalTexture(whiteTexture);
					}

					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path scenePath = g_AssetPath / path;

						std::shared_ptr<Texture2D> normalTex = AssetManager::GetAsset<Texture2D>(scenePath.string());
						vulkanMaterial->SetNormalTexture(normalTex);
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				ImGui::Checkbox("Use", (bool*)&materialData.UseNormalMap);

				ImGui::TreePop();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 4 });
			bool armNode = ImGui::TreeNodeEx("ROUGHNESS/METALLIC", treeNodeFlags);
			ImGui::PopStyleVar();

			if (armNode)
			{
				ImGui::Image((ImTextureID)arm, { 100.0f, 100.0f }, { 0, 1 }, { 1, 0 });

				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
					ImGui::OpenPopup("RemoveTexture");

				if (ImGui::BeginPopup("RemoveTexture"))
				{
					if (ImGui::MenuItem("Remove Texture"))
					{
						auto whiteTexture = Renderer::GetWhiteTexture(ImageFormat::RGBA8_UNORM);
						vulkanMaterial->SetARMTexture(whiteTexture);
					}

					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						const wchar_t* path = (const wchar_t*)payload->Data;
						std::filesystem::path scenePath = g_AssetPath / path;

						std::shared_ptr<Texture2D> armTex = AssetManager::GetAsset<Texture2D>(scenePath.string());
						vulkanMaterial->SetARMTexture(armTex);
					}

					ImGui::EndDragDropTarget();
				}

				ImGui::DragFloat("Roughness", &materialData.Roughness, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Metallic", &materialData.Metallic, 0.01f, 0.0f, 1.0f);

				ImGui::TreePop();
			}

			ImGui::End();
		}
	}

}
