#include "SceneHierarchyPanel.h"

#include "VulkanCore/Asset/AssetManager.h"
#include "VulkanCore/Asset/MaterialAsset.h"
#include "VulkanCore/Mesh/Mesh.h"
#include "VulkanCore/Scene/SceneRenderer.h"
#include "VulkanCore/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanMaterial.h"

#include <filesystem>

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>

namespace VulkanCore {

	static const std::filesystem::path g_AssetPath = "assets";

	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[1];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.6022f, 0.00533f, 0.0134f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.787f, 0.0257f, 0.0257f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.6022f, 0.00533f, 0.0134f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0257f, 0.4445f, 0.0257f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.0648f, 0.6022f, 0.0648f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.0257f, 0.4445f, 0.0257f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.00533f, 0.0428f, 0.6022f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.0257f, 0.092f, 0.787f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.00533f, 0.0428f, 0.6022f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	SceneHierarchyPanel::SceneHierarchyPanel(std::shared_ptr<Scene> sceneContext)
		: m_Context(sceneContext)
	{
	}

	SceneHierarchyPanel::~SceneHierarchyPanel()
	{
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy Panel");

		if (m_Context)
		{
			m_Context->m_Registry.each([&](auto entityID)
			{
				Entity entity{ entityID, m_Context.get() };
				DrawEntityNode(entity);
			});

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectionContext = {};

			// Right-click on blank space
			if (ImGui::BeginPopupContextWindow("##CreateEntity", 1))
			{
				if (ImGui::MenuItem("Create Empty Entity"))
					m_Context->CreateEntity("Empty Entity");

				ImGui::EndPopup();
			}
		}

		ImGui::End(); // End of Scene Hierarchy Panel

		ImGui::Begin("Properties");
		if (m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
		}

		ImGui::End(); // End of Properties Panel
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{

	}

	void SceneHierarchyPanel::SetContext(std::shared_ptr<Scene> context)
	{
		m_Context = context;
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (opened)
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			bool opened = ImGui::TreeNodeEx((void*)9817239, flags, tag.c_str());
			if (opened)
			{
				if (entity.HasComponent<MeshComponent>())
				{
					MeshComponent meshComponent = entity.GetComponent<MeshComponent>();
					
					for (const MeshNode& submeshes : meshComponent.MeshInstance->GetMeshSource()->GetMeshNodes())
					{
						if (ImGui::TreeNode(submeshes.Name.c_str()))
							ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleVar();

			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("-", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent)
				entity.RemoveComponent<T>();
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			std::strncpy(buffer, tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent"))
		{
			DisplayAddComponentEntry<PointLightComponent>("Point Light");
			DisplayAddComponentEntry<SpotLightComponent>("Spot Light");
			DisplayAddComponentEntry<MeshComponent>("Mesh");

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
		{
			DrawVec3Control("Translation", component.Translation);
			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);
			DrawVec3Control("Scale", component.Scale, 1.0f);
		});

		DrawComponent<PointLightComponent>("Point Light", entity, [](auto& component)
		{
			ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Intensity", (float*)&component.Color.w, 0.01f, 0.0f, 10000.0f);
			ImGui::DragFloat("Falloff", &component.Falloff, 0.01f, 0.0f, 10000.0f);
			ImGui::DragFloat("Radius", &component.Radius, 0.01f, 0.001f, 1000.0f);
		});

		DrawComponent<SpotLightComponent>("Spot Light", entity, [](auto& component)
		{
			ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Intensity", (float*)&component.Color.w, 0.01f, 0.0f, 10000.0f);
			float innerCutoff = glm::degrees(component.InnerCutoff);
			float outerCutoff = glm::degrees(component.OuterCutoff);
			ImGui::DragFloat("Inner Cutoff", &innerCutoff, 0.01f, 0.01f, outerCutoff);
			ImGui::DragFloat("Outer Cutoff", &outerCutoff, 0.01f, innerCutoff, 80.0f);
			component.InnerCutoff = glm::radians(innerCutoff);
			component.OuterCutoff = glm::radians(outerCutoff);

			ImGui::DragFloat("Falloff", &component.Falloff, 0.01f, 0.0f, 10000.0f);
			ImGui::DragFloat("Radius", &component.Radius, 0.01f, 0.001f, 1000.0f);
		});

		DrawComponent<MeshComponent>("Mesh", entity, [](auto& component)
		{
			// Mesh Asset
			auto meshSource = component.MeshInstance->GetMeshSource();
			auto sceneRenderer = SceneRenderer::GetSceneRenderer();

			AssetHandle meshHandle = meshSource->Handle;
			auto& meshAssetMetadata = AssetManager::GetAssetMetadata(meshHandle);
			auto meshAssetPath = meshAssetMetadata.FilePath.generic_string();
			ImGui::InputText("Mesh", meshAssetPath.data(), meshAssetPath.size(), ImGuiInputTextFlags_ReadOnly);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* path = (const wchar_t*)payload->Data;
					std::filesystem::path assetPath = g_AssetPath / path;

					std::shared_ptr<Mesh> oldMesh = component.MeshInstance;
					sceneRenderer->UpdateMeshInstanceData(oldMesh, component.MaterialTable);

					std::shared_ptr<Mesh> mesh = AssetManager::GetAsset<Mesh>(assetPath.string());
					component.MeshInstance = mesh;
				}

				ImGui::EndDragDropTarget();
			}

			ImGui::Separator();

			// Material Asset
			AssetHandle materialHandle = component.MaterialTable->Handle;
			auto& materialAssetMetadata = AssetManager::GetAssetMetadata(materialHandle);
			auto materialAssetPath = materialAssetMetadata.FilePath.generic_string();
			ImGui::InputText("Material", materialAssetPath.data(), materialAssetPath.size(), ImGuiInputTextFlags_ReadOnly);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* path = (const wchar_t*)payload->Data;
					std::filesystem::path assetPath = g_AssetPath / path;

					std::shared_ptr<MaterialAsset> oldMaterialAsset = component.MaterialTable;
					sceneRenderer->UpdateMeshInstanceData(component.MeshInstance, oldMaterialAsset);

					std::shared_ptr<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(assetPath.string());
					component.MaterialTable = materialAsset;
					meshSource->SetMaterial(materialAsset->GetMaterial());
				}

				ImGui::EndDragDropTarget();
			}

			ImGui::Separator();

			// Mesh Stats
			ImGui::Text("Vertex Count: %d", meshSource->GetVertexCount());
			ImGui::Text("Index Count: %d", meshSource->GetIndexCount());
		});
	}

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName)
	{
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}
}