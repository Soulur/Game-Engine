#include "SceneHierarchyPanel.h"
#include "src/Scene/Components.h"

#include "src/Renderer/Manager/ModelManager.h"
// #include "src/Scripting/ScriptEngine.h"


#include <imgui.h>
#include <imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <filesystem>

#include <entt/entt.hpp>

/* The Microsoft C++ compiler is non-compliant with the C++ standard and needs
 * the following definition to disable a security warning on std::strncpy().
 */
#ifdef _MSVC_LANG
  #define _CRT_SECURE_NO_WARNINGS
#endif

namespace Mc {

	extern const std::filesystem::path g_AssetPath;

	// 支持文件类型
	extern const std::vector<std::string> TextureSupportType{".png", ".jpg"};

	extern const std::vector<std::string> ModelSupportType{".obj"};

	extern const std::vector<std::string> HdrSupportType{".hdr"};

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");

		if (m_Context)
		{
			auto view = m_Context->m_Registry.view<entt::entity>();
			for (auto entityID : view)
			{
				Entity entity{entityID, m_Context.get()};
				DrawEntityNode(entity);
			}

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectionContext = {};

			// Right-click on blank space
			if (ImGui::BeginPopupContextWindow(0, 1, false))
			{
				if (ImGui::MenuItem ("Create Empty Entity"))
					m_Context->CreateEntity("Empty Entity");

				ImGui::EndPopup();
			}
		}
		ImGui::End();

		ImGui::Begin("Properties");
		if (m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
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
				ImGui::TreePop();
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
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
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
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

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entry)
	{
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entry.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
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
			DisplayAddComponentEntry<TransformComponent>("Transform");
			DisplayAddComponentEntry<CameraComponent>("Camera");
			DisplayAddComponentEntry<LightComponent>("Light Renderer");
			DisplayAddComponentEntry<SphereRendererComponent>("Sphere Renderer");
			DisplayAddComponentEntry<ModelRendererComponent>("Model Renderer");
			DisplayAddComponentEntry<HdrSkyboxComponent>("Hdr Skybox");

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

		DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
		{
			auto& camera = component.Camera;

			ImGui::Checkbox("Primary", &component.Primary);

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						camera.SetProjectionType((SceneCamera::ProjectionType)i);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float perspectiveVerticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
					camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));

				float perspectiveNear = camera.GetPerspectiveNearClip();
				if (ImGui::DragFloat("Near", &perspectiveNear))
					camera.SetPerspectiveNearClip(perspectiveNear);

				float perspectiveFar = camera.GetPerspectiveFarClip();
				if (ImGui::DragFloat("Far", &perspectiveFar))
					camera.SetPerspectiveFarClip(perspectiveFar);
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if (ImGui::DragFloat("Size", &orthoSize))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if (ImGui::DragFloat("Near", &orthoNear))
					camera.SetOrthographicNearClip(orthoNear);

				float orthoFar = camera.GetOrthographicFarClip();
				if (ImGui::DragFloat("Far", &orthoFar))
					camera.SetOrthographicFarClip(orthoFar);

				ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
			}
		});

		DrawComponent<LightComponent>("Light Renderer", entity, [](auto &component)
		{
			SceneLight& light = component.Light;

			const char *projectionTypeStrings[] = {"Point Light", "Directional Light", "Spot Light"};
			const char *currentProjectionTypeString = projectionTypeStrings[(int)light.GetType()];
			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 3; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						light.SetType((SceneLight::LightType)i);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (ImGui::ColorEdit3("Color", glm::value_ptr(component.Color)))
			{
				auto color = component.Color;
				light.SetColor(color);
			}
			ImGui::DragFloat("Intensity", &component.Intensity);
			{
				auto intensity = component.Intensity;
				light.SetIntensity(intensity);
			}
			ImGui::Checkbox("CastsShadows", &component.CastsShadows);

			if (light.GetType() == SceneLight::LightType::Point)
			{
				float radius = light.GetRadius();
				// 半径：建议最小值0.1（避免光源退化），最大值根据场景需求，例如500.0f
				// 拖动速度：0.1f 提供平滑调整，如果需要更粗略调整可设为1.0f
				if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 500.0f, "%.1f"))
					light.SetRadius(radius);
			}

			// --- 聚光灯属性 ---
			if (light.GetType() == SceneLight::LightType::Spot)
			{
				float radius = light.GetRadius();
				if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 500.0f, "%.1f"))
					light.SetRadius(radius);
				// 内锥角：
				// 范围：0.0f 到 89.0f (必须小于外锥角，且小于90度)
				// 拖动速度：0.1f 提供平滑调整
				float innerConeAngle = light.GetInnerConeAngleDegrees();
				if (ImGui::DragFloat("InnerConeAngle (Deg)", &innerConeAngle, 0.1f, 0.0f, 89.0f, "%.1f"))
					light.SetInnerConeAngleDegrees(innerConeAngle);

				// 外锥角：
				// 范围：1.0f 到 90.0f (必须大于内锥角，且小于等于90度)
				// 拖动速度：0.1f 提供平滑调整
				float outerConeAngle = light.GetOuterConeAngleDegrees();
				if (ImGui::DragFloat("OuterConeAngle (Deg)", &outerConeAngle, 0.1f, 1.0f, 90.0f, "%.1f"))
					light.SetOuterConeAngleDegrees(outerConeAngle);
			}
		});

		DrawComponent<SphereRendererComponent>("Sphere Renderer", entity, [](auto &component) 
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			
			ImGui::DragFloat("Tiling Texture", &component.TilingFactor, 0.1f, 0.0f, 100.0f);

			ImGui::Text("PBR");
			{
				glm::vec4 albedo = component.Material->GetAlbedo();
				ImGui::ColorEdit4("Albedo", glm::value_ptr(albedo));
				component.Material->SetAlbedo(albedo);
			}

			{
				auto roughness = component.Material->GetRoughness();
				ImGui::DragFloat("Roughness", &roughness, 0.1f, 0.0f, 1.0f);
				component.Material->SetRoughness(roughness);
			}

			{
				auto metallic = component.Material->GetMetallic();
				ImGui::DragFloat("Metallic", &metallic, 0.1f, 0.0f, 1.0f);
				component.Material->SetMetallic(metallic);
			}

			{
				auto ao = component.Material->GetAO();
				ImGui::DragFloat("AO", &ao, 0.1f, 0.0f, 1.0f);
				component.Material->SetAO(ao);
			}

			{
				auto emissive = component.Material->GetEmissive();
				ImGui::ColorEdit3("Emissive", glm::value_ptr(emissive));
				component.Material->SetEmissive(emissive);
			}

			// Ref<Texture2D> Albedo = 			component.Material->GetTexture(TextureType::Albedo);
			// Ref<Texture2D> Normal = 			component.Material->GetTexture(TextureType::Normal);
			// Ref<Texture2D> Metallic = 			component.Material->GetTexture(TextureType::Metallic);
			// Ref<Texture2D> Roughness = 			component.Material->GetTexture(TextureType::Roughness);
			// Ref<Texture2D> AmbientOcclusion = 	component.Material->GetTexture(TextureType::AmbientOcclusion);
			// Ref<Texture2D> Emissive = 			component.Material->GetTexture(TextureType::Emissive);
			// Ref<Texture2D> Height = 			component.Material->GetTexture(TextureType::Height);

			ImGui::Text("Albedo");
			 if (ImGui::Button(component.Material->GetTexture(TextureType::Albedo)->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
			ImGui::Text("Normal");
			 if (ImGui::Button(component.Material->GetTexture(TextureType::Normal)->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
			ImGui::Text("Metallic");
			 if (ImGui::Button(component.Material->GetTexture(TextureType::Metallic)->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
			ImGui::Text("Roughness");
			 if (ImGui::Button(component.Material->GetTexture(TextureType::Roughness)->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
			ImGui::Text("AmbientOcclusion");
			 if (ImGui::Button(component.Material->GetTexture(TextureType::AmbientOcclusion)->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
			ImGui::Text("Emissive");
			 if (ImGui::Button(component.Material->GetTexture(TextureType::Emissive)->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
			ImGui::Text("Height");
			 if (ImGui::Button(component.Material->GetTexture(TextureType::Height)->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
		});

		DrawComponent<ModelRendererComponent>("Model Renderer", entity, [](auto &component)
		{
			ImGui::Text("Model Path");
			ImGui::Button(component.ModelPath.c_str(), ImVec2(ImGui::GetWindowSize().x, 40));

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("FOLDER_PANEL"))
				{
					const wchar_t* path = (const wchar_t*)payload->Data;
					std::filesystem::path modelPath = std::filesystem::path(g_AssetPath) / path;
					std::string extension = modelPath.extension().string();

					bool nonsupport = false;
					for (auto &type : ModelSupportType)
						if (type == extension)
						{
							nonsupport = true;
							break;
						}
					if (nonsupport)
					{
						component.ModelPath = modelPath.string();
						component.Model = ModelManager::Get().GetModel(modelPath.string());
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

			if (ImGui::Checkbox("FlipUV", &component.FlipUV))
			{
				if (component.Model != nullptr)
					component.Model->SetFlipUV(component.FlipUV);
			}
			ImGui::Checkbox("GammaCorrection", &component.GammaCorrection);

			ImGui::Text("Meshs");
			{
				ImGui::BeginChild("Meshs", ImVec2(0, 200), true, ImGuiWindowFlags_NoScrollbar); // 创建一个子区域
				if (!component.ModelPath.empty() && component.Model != nullptr)
					for (auto &obj : component.Model->GetMeshs())
					{
						if (ImGui::Button(obj->GetName().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)))
						{
							component.CurrentMesh = obj;
						}
					}
				ImGui::EndChild();
			}

			ImGui::Text("Selected Mesh Properties");
			ImGui::BeginChild("Selected Mesh Properties", ImVec2(0, 700), true, ImGuiWindowFlags_NoScrollbar); // 创建一个子区域
			if (component.CurrentMesh != nullptr)
			{
				ImGui::Text(component.CurrentMesh->GetName().c_str());

				// ImGui::PushItemWidth(-1);

				if (ImGui::Button("Add Materials"))
					ImGui::OpenPopup("AddMaterials");

				if (ImGui::BeginPopup("AddMaterials"))
				{
					for (auto &obj : component.Model->GetMaterials())
					{
						if (ImGui::MenuItem(obj->GetName().c_str()))
						{
							LOG_CORE_WARN("{0}", obj->GetName().c_str());
							component.CurrentMesh->SetMaterial(obj);
						}
					}

					ImGui::EndPopup();
				}

				ImGui::Text(component.CurrentMesh->GetMaterial()->GetName().c_str());

				glm::vec4 albedo = component.CurrentMesh->GetMaterial()->GetAlbedo();
				ImGui::ColorEdit4("Albedo", glm::value_ptr(albedo));

				auto roughness = component.CurrentMesh->GetMaterial()->GetRoughness();
				ImGui::DragFloat("Roughness", &roughness);

				auto metallic = component.CurrentMesh->GetMaterial()->GetMetallic();
				ImGui::DragFloat("Metallic", &metallic);

				auto ao = component.CurrentMesh->GetMaterial()->GetAO();
				ImGui::DragFloat("AO", &ao);

				auto emissive = component.CurrentMesh->GetMaterial()->GetEmissive();
				ImGui::ColorEdit3("Emissive", glm::value_ptr(emissive));

				Ref<Texture2D> Albedo = 			component.CurrentMesh->GetMaterial()->GetTexture(TextureType::Albedo);
				Ref<Texture2D> Normal = 			component.CurrentMesh->GetMaterial()->GetTexture(TextureType::Normal);
				Ref<Texture2D> Metallic = 			component.CurrentMesh->GetMaterial()->GetTexture(TextureType::Metallic);
				Ref<Texture2D> Roughness = 			component.CurrentMesh->GetMaterial()->GetTexture(TextureType::Roughness);
				Ref<Texture2D> AmbientOcclusion = 	component.CurrentMesh->GetMaterial()->GetTexture(TextureType::AmbientOcclusion);
				Ref<Texture2D> Emissive = 			component.CurrentMesh->GetMaterial()->GetTexture(TextureType::Emissive);
				Ref<Texture2D> Height = 			component.CurrentMesh->GetMaterial()->GetTexture(TextureType::Height);

				ImGui::Text("Albedo");
				if (ImGui::Button(Albedo->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
				ImGui::Text("Normal");
				if (ImGui::Button(Normal->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
				ImGui::Text("Metallic");
				if (ImGui::Button(Metallic->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
				ImGui::Text("Roughness");
				if (ImGui::Button(Roughness->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
				ImGui::Text("AmbientOcclusion");
				if (ImGui::Button(AmbientOcclusion->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
				ImGui::Text("Emissive");
				if (ImGui::Button(Emissive->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
				ImGui::Text("Height");
				if (ImGui::Button(Height->GetPath().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)));
			}
			ImGui::EndChild();
		});

		DrawComponent<HdrSkyboxComponent>("Hdr Skybox", entity, [](auto &component)
		{
			ImGui::Text("Hdr Skybox");
			ImGui::Button(component.Path.c_str(), ImVec2(ImGui::GetWindowSize().x, 40));

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("FOLDER_PANEL"))
				{
					const wchar_t *path = (const wchar_t *)payload->Data;
					std::filesystem::path hdrPath = std::filesystem::path(g_AssetPath) / path;
					std::string extension = hdrPath.extension().string();

					bool nonsupport = false;
					for (auto &type : HdrSupportType)
						if (type == extension)
						{
							nonsupport = true;
							break;
						}
					if (nonsupport)
					{
						component.Path = hdrPath.string();
					}
				}
				ImGui::EndDragDropTarget();
			}
		});

		// DrawComponent<CircleRendererComponent>("Circle Renderer", entity, [](auto& component)
		// {
		// 	ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
		// 	ImGui::DragFloat("TIickness", &component.Thickness, 0.025f, 0.0f, 1.0f);
		// 	ImGui::DragFloat("Fade", &component.Fade, 0.00025f, 0.0f, 1.0f);
		// });
	}
}
