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

	namespace Utils
	{
		void PbrUiRenderer(Ref<Material> &material)
		{
			ImGui::Text("PBR");
			{
				glm::vec4 albedo = material->GetAlbedo();
				ImGui::ColorEdit4("Albedo", glm::value_ptr(albedo));
				material->SetAlbedo(albedo);
			}

			{
				auto roughness = material->GetRoughness();
				ImGui::DragFloat("Roughness", &roughness, 0.1f, 0.0f, 1.0f);
				material->SetRoughness(roughness);
			}

			{
				auto metallic = material->GetMetallic();
				ImGui::DragFloat("Metallic", &metallic, 0.1f, 0.0f, 1.0f);
				material->SetMetallic(metallic);
			}

			{
				auto ao = material->GetAO();
				ImGui::DragFloat("AO", &ao, 0.1f, 0.0f, 1.0f);
				material->SetAO(ao);
			}

			{
				auto emissive = material->GetEmissive();
				ImGui::ColorEdit3("Emissive", glm::value_ptr(emissive));
				material->SetEmissive(emissive);
			}

			std::vector<std::string> arr = {"AlbedoMap", "NormalMap", "MetallicMap", "RoughnessMap", "AmbientOcclusionMap", "EmissiveMap", "HeightMap"};

			for (int i = 0; i < (int)TextureType::Count; i++)
			{
				ImGui::PushID(i);

				Ref<Texture2D> texture = material->GetTexture((TextureType)i);

				std::string buttonLabel;
				if (texture && !texture->GetPath().empty())
					buttonLabel = texture->GetPath();
				else buttonLabel = arr[i] + " (None)";

				ImGui::Text(arr[i].c_str());

				if (ImGui::Button(buttonLabel.c_str(), ImVec2(ImGui::GetWindowSize().x, 40)))
				{
				}
				ImGui::PopID();
			}
		}
	}

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
			if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
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
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	static void DrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO &io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		if (ImGui::BeginTable("##VectorsTable", 2, ImGuiTableFlags_NoBordersInBody))
		{
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
			ImGui::TableSetupColumn("Values");

			// 切换到第一列，并绘制标签
			ImGui::TableNextColumn();
			ImGui::Text(label.c_str());

			// 切换到第二列，并绘制输入控件
			ImGui::TableNextColumn();
			ImGui::PushMultiItemsWidths(4, ImGui::GetContentRegionAvail().x);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

			float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
			ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

			// X value
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
			ImGui::PushFont(boldFont);
			if (ImGui::Button("X", buttonSize))
				values.x = resetValue;
			ImGui::PopFont();
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
			ImGui::SameLine();

			// Y value
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
			ImGui::PushFont(boldFont);
			if (ImGui::Button("Y", buttonSize))
				values.y = resetValue;
			ImGui::PopFont();
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
			ImGui::SameLine();

			// Z value
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
			ImGui::PushFont(boldFont);
			if (ImGui::Button("Z", buttonSize))
				values.z = resetValue;
			ImGui::PopFont();
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();

			ImGui::PopStyleVar();

			ImGui::EndTable();
		}

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
			float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
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
			DisplayAddComponentEntry<DirectionalLightComponent>("Directional Light");
			DisplayAddComponentEntry<PointLightComponent>("Point Light");
			DisplayAddComponentEntry<SpotLightComponent>("Spot Light");

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

		DrawComponent<DirectionalLightComponent>("Directional Light", entity, [this](auto &component)
		{	
			ImGui::Text("Directional Light");

			ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Intensity", &component.Intensity, 0.1f, 0.0f, 10.0f, "%.1f");

			ImGui::Checkbox("Casts Shadows", &component.CastsShadows);

			if (component.CastsShadows)
			{
				if (!m_SelectionContext.HasComponent<ShadowComponent>())
				{
					m_SelectionContext.AddComponent<ShadowComponent>();
				}
			}
			else
			{
				if (m_SelectionContext.HasComponent<ShadowComponent>())
				{
					m_SelectionContext.RemoveComponent<ShadowComponent>();
				}
			}
		});

		DrawComponent<PointLightComponent>("Point Light", entity, [this](auto &component)
		{	
			ImGui::Text("Point Light Properties");

			ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Intensity", &component.Intensity, 0.1f, 0.0f, 10.0f, "%.1f");
			ImGui::DragFloat("Radius", &component.Radius, 0.1f, 0.1f, 500.0f, "%.1f");

			ImGui::Checkbox("Casts Shadows", &component.CastsShadows);

			if (component.CastsShadows)
			{
				if (!m_SelectionContext.HasComponent<ShadowComponent>())
				{
					m_SelectionContext.AddComponent<ShadowComponent>();
				}
			}
			else
			{
				if (m_SelectionContext.HasComponent<ShadowComponent>())
				{
					m_SelectionContext.RemoveComponent<ShadowComponent>();
				}
			}
		});

		DrawComponent<SpotLightComponent>("Spot Light", entity, [](auto &component)
		{
			ImGui::Text("Spot Light Properties");

			ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Intensity", &component.Intensity, 0.1f, 0.0f, 10.0f, "%.1f");
			ImGui::DragFloat("Radius", &component.Radius, 0.1f, 0.1f, 500.0f, "%.1f");

			// 内锥角
			float innerConeAngle = glm::degrees(component.InnerConeAngle);
			if (ImGui::DragFloat("InnerConeAngle (Deg)", &innerConeAngle, 0.1f, 0.0f, 89.0f, "%.1f"))
				component.InnerConeAngle = glm::radians(innerConeAngle);

			// 外锥角
			float outerConeAngle = glm::degrees(component.OuterConeAngle);
			if (ImGui::DragFloat("OuterConeAngle (Deg)", &outerConeAngle, 0.1f, 1.0f, 90.0f, "%.1f"))
				component.OuterConeAngle = glm::radians(outerConeAngle);
		});

		DrawComponent<ShadowComponent>("Shadow Renderer", entity, [](auto &component)
		{	
			ImGui::Text("Shadow");

			ImGui::InputInt("Resolution", (int *)&component.Resolution);
			ImGui::DragFloat("Near Plane", &component.NearPlane, 0.1f, 0.0f, 1000.0f, "%.1f");
			ImGui::DragFloat("Far Plane", &component.FarPlane, 0.1f, 0.0f, 10000.0f, "%.1f");
		});

		DrawComponent<SphereRendererComponent>("Sphere Renderer", entity, [](auto &component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Tiling Texture", &component.TilingFactor, 0.1f, 0.0f, 100.0f);

			ImGui::Checkbox("ReceivesPBR", &component.ReceivesPBR);
			ImGui::Checkbox("ReceivesIBL", &component.ReceivesIBL);
			ImGui::Checkbox("ReceivesLight", &component.ReceivesLight);
			ImGui::Checkbox("ReceivesShadow", &component.ReceivesShadow);
			ImGui::Checkbox("ProjectionShadow", &component.ProjectionShadow);

			Utils::PbrUiRenderer(component.Material); 
		});

		DrawComponent<ModelRendererComponent>("Model Renderer", entity, [](auto &component)
		{
			ImGui::Text("Model Path");
			ImGui::Button((component.ModelPath + "##ModelPath").c_str(), ImVec2(ImGui::GetWindowSize().x, 40));

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

			ImGui::Checkbox("FlipUV", &component.FlipUV);

			ImGui::Checkbox("ReceivesPBR", &component.ReceivesPBR);
			ImGui::Checkbox("ReceivesIBL", &component.ReceivesIBL);
			ImGui::Checkbox("ReceivesLight", &component.ReceivesLight);

			ImGui::Checkbox("GammaCorrection", &component.GammaCorrection);

			ImGui::Text("Meshs");
			{
				ImGui::BeginChild("Meshs", ImVec2(0, 200), true, ImGuiWindowFlags_NoScrollbar); // 创建一个子区域
				if (!component.ModelPath.empty() && component.Model != nullptr)
				{
					for (auto &obj : component.Model->GetMeshs())
					{
						if (ImGui::Button(obj->GetName().c_str(), ImVec2(ImGui::GetWindowSize().x, 40)))
						{
							component.CurrentMesh = obj;
						}
					}
				}
				ImGui::EndChild();
			}

			ImGui::Text("Selected Mesh Properties");
			ImGui::BeginChild("Selected Mesh Properties", ImVec2(0, 700), true, ImGuiWindowFlags_NoScrollbar); // 创建一个子区域
			if (component.CurrentMesh != nullptr)
			{
				ImGui::Text(component.CurrentMesh->GetName().c_str());

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

				Utils::PbrUiRenderer(component.CurrentMesh->GetMaterial());
			}
			ImGui::EndChild();
		});

		DrawComponent<HdrSkyboxComponent>("Hdr Skybox", entity, [](auto &component)
		{
			ImGui::Text("Hdr Skybox");
			ImGui::Button((component.Path + "##HdrPath").c_str(), ImVec2(ImGui::GetWindowSize().x, 40));

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
