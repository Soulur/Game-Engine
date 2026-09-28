#include "SceneHierarchyPanel.h"
#include "src/Scene/Components.h"

#include "src/Renderer/Manager/ModelManager.h"
#include "src/Renderer/Manager/TextureManager.h"
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

	extern const std::vector<std::string> ModelSupportType{".obj", ".fbx"};

	extern const std::vector<std::string> HdrSupportType{".hdr"};

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};

		m_PreviewSize = 300.0f;
		m_ShowMaterialEditor = false;
		FramebufferSpecification fbSpec;
		fbSpec.Attachments = {FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::DEPTH24STENCIL8};
		fbSpec.Width = m_PreviewSize;
		fbSpec.Height = m_PreviewSize;
		m_PreviewFramebuffer = Framebuffer::Create(fbSpec);
		m_PreviewMaterialCamera = EditorCamera(30.0f, 1.778f, 0.1f, 100.0f);

		m_DefaultTexture = Texture2D::Create("Resources/Textures/Default-texture.png");
		m_VisibleIcon = Texture2D::Create("Resources/Icons/eye-outline.png");
		m_VisibleOffIcon = Texture2D::Create("Resources/Icons/eye-off-outline.png");
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");

		if (m_Context)
		{
			ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_Reorderable |
										   ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
			// ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
			// 						ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInnerV |
			// 						ImGuiTableFlags_ScrollY;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);

			if (ImGui::BeginTable("HierarchyTable", 3, flags))
			{
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
				ImGui::TableSetupColumn("Visibility", ImGuiTableColumnFlags_WidthFixed, 30.0f);
				ImGui::TableHeadersRow();

				auto view = m_Context->m_Registry.view<entt::entity>();
				for (auto entityID : view)
				{
					Entity entity{entityID, m_Context.get()};

					bool isRoot = true;
					if (entity.HasComponent<HierarchyComponent>())
						if (entity.GetComponent<HierarchyComponent>().Parent != 0)
							isRoot = false;

					if (isRoot)
						DrawEntityNode(entity);
				}

				{
					if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
						m_SelectionContext = {};

					if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
					{
						if (ImGui::MenuItem("Create Empty Entity"))
							m_Context->CreateEntity("Empty Entity");
						ImGui::EndPopup();
					}
				}

				ImGui::EndTable();
				ImGui::PopStyleVar(3);
			}
		}
		ImGui::End();

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
		ImGui::Begin("Properties");
		ImGui::PopStyleColor();
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

	bool SceneHierarchyPanel::IsParentVisible(Entity entity)
	{
		if (!entity || !entity.HasComponent<HierarchyComponent>())
			return true;

		UUID parentUUID = entity.GetComponent<HierarchyComponent>().Parent;
		if (parentUUID == 0)
			return true;

		Entity parent = m_Context->GetEntityByUUID(parentUUID);

		if (parent)
		{
			if (parent.HasComponent<VisibleComponent>())
			{
				if (!parent.GetComponent<VisibleComponent>().Visible)
					return false;
			}
			return IsParentVisible(parent);
		}

		return true;
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		bool isSelected = (m_SelectionContext == entity);

		float lineHeight = ImGui::GetTextLineHeightWithSpacing() + 2.0f;
		ImGui::TableNextRow(ImGuiTableRowFlags_None, lineHeight);
		ImGui::TableSetColumnIndex(0);

		ImGui::PushID((int)(uint32_t)entity);
		ImGuiID nodeID = ImGui::GetID((void *)(uint64_t)(uint32_t)entity);

		bool hasChildren = false;
		if (entity.HasComponent<HierarchyComponent>())
			hasChildren = !entity.GetComponent<HierarchyComponent>().Children.empty();

		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (!hasChildren)
			treeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

		bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, treeFlags, "");

		ImGui::PopStyleColor(3);
		ImGui::SameLine();

		ImGuiSelectableFlags selFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 0.0f));
		if (ImGui::Selectable(tag.c_str(), isSelected, selFlags, ImVec2(0, lineHeight)))
			m_SelectionContext = entity;
		ImGui::PopStyleVar();

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
		{
			ImGuiStorage *storage = ImGui::GetStateStorage();
			bool *p_open = storage->GetBoolRef(nodeID);
			*p_open = !*p_open;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
				m_Context->DestroyEntity(entity);
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(3);

		if (ImGui::TableSetColumnIndex(1))
		{
			ImGui::AlignTextToFramePadding();
			std::string typeString = "Entity";

			if (entity.HasComponent<MeshRendererComponent>())
				typeString = "Mesh";
			else if (entity.HasComponent<DirectionalLightComponent>() || entity.HasComponent<PointLightComponent>() || entity.HasComponent<SpotLightComponent>())
				typeString = "Light";
			else if (entity.HasComponent<ModelRendererComponent>())
				typeString = "Model";
			else if (entity.HasComponent<SphereRendererComponent>())
				typeString = "Sphere";
			else if (entity.HasComponent<CameraComponent>())
				typeString = "Camera";
			else if (entity.HasComponent<HdrSkyboxComponent>())
				typeString = "Hdr";

			ImGui::TextDisabled("%s", typeString.c_str());
		}
		if (ImGui::TableSetColumnIndex(2))
		{
			ImGui::AlignTextToFramePadding();

			
			if (entity.HasComponent<VisibleComponent>())
			{
				bool &selfVisible = entity.GetComponent<VisibleComponent>().Visible;
				bool parentVisible = IsParentVisible(entity);

				bool effectivelyVisible = selfVisible && parentVisible;
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

				ImTextureID icon = effectivelyVisible ? m_VisibleIcon->GetRendererID() : m_VisibleOffIcon->GetRendererID();

				if (ImGui::ImageButton("##Visible", icon, ImVec2(lineHeight - 5.0f, lineHeight - 5.0f), ImVec2(0, 1), ImVec2(1, 0)))
				{
					selfVisible = !selfVisible;
				}

				ImGui::PopStyleColor();
			}
		}

		ImGui::PopID();

		if (opened && hasChildren)
		{
			for (Entity childEntity : entity.GetChildren())
				DrawEntityNode(childEntity);
			ImGui::TreePop();
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
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
			ImGui::AlignTextToFramePadding();
			ImGui::PushFont(boldFont);
			ImGui::Text("X");
			ImGui::PopFont();
			ImGui::PopStyleColor();

			ImGui::SameLine(0, 5.0f);
			ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
			ImGui::SameLine(0, 5.0f);

			// Y value
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
			ImGui::AlignTextToFramePadding();
			ImGui::PushFont(boldFont);
			ImGui::Text("Y");
			ImGui::PopFont();
			ImGui::PopStyleColor();

			ImGui::SameLine(0, 5.0f);
			ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
			ImGui::SameLine(0, 5.0f);

			// Z value
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
			ImGui::AlignTextToFramePadding();
			ImGui::PushFont(boldFont);
			ImGui::Text("Z");
			ImGui::PopFont();
			ImGui::PopStyleColor();

			ImGui::SameLine(0, 5.0f);
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

			ImGui::SameLine(contentRegionAvailable.x - lineHeight);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}
			ImGui::PopStyleVar(3);

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

		ImGui::SameLine();
		if (ImGui::Button("Vis")){}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);

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

		ImGui::PopStyleVar(3);

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

		DrawComponent<SpotLightComponent>("Spot Light", entity, [this](auto &component)
		{
			ImGui::Text("Spot Light Properties");

			ImGui::ColorEdit3("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Intensity", &component.Intensity, 0.1f, 0.0f, 10.0f, "%.1f");
			ImGui::DragFloat("Radius", &component.Radius, 0.1f, 0.1f, 500.0f, "%.1f");

			float innerConeAngle = glm::degrees(component.InnerConeAngle);
			if (ImGui::DragFloat("InnerConeAngle (Deg)", &innerConeAngle, 0.1f, 0.0f, 89.0f, "%.1f"))
			{
				if (innerConeAngle > glm::degrees(component.OuterConeAngle))
					innerConeAngle = glm::min(innerConeAngle, glm::degrees(component.OuterConeAngle) - 1.0f);
				component.InnerConeAngle = glm::radians(innerConeAngle);
			}

			float outerConeAngle = glm::degrees(component.OuterConeAngle);
			if (ImGui::DragFloat("OuterConeAngle (Deg)", &outerConeAngle, 0.1f, 1.0f, 90.0f, "%.1f"))
			{
				if (outerConeAngle < glm::degrees(component.InnerConeAngle))
					outerConeAngle = glm::degrees(component.InnerConeAngle) + 1.0f;
				component.OuterConeAngle = glm::radians(outerConeAngle);
			}

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

		DrawComponent<ShadowComponent>("Shadow Renderer", entity, [](auto &component)
		{	
			ImGui::Text("Shadow");

			ImGui::InputInt("Resolution", (int *)&component.Resolution);
			ImGui::DragFloat("Near Plane", &component.NearPlane, 0.1f, 0.0f, 1000.0f, "%.1f");
			ImGui::DragFloat("Far Plane", &component.FarPlane, 0.1f, 0.0f, 10000.0f, "%.1f");
		});

		DrawComponent<SphereRendererComponent>("Sphere Renderer", entity, [this](auto &component)
		{
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));
			ImGui::DragFloat("Tiling Texture", &component.TilingFactor, 0.1f, 0.0f, 100.0f);

			ImGui::Checkbox("IsMaterial", &component.IsMaterial);
			if (component.IsMaterial)
			{
				if (!m_SelectionContext.HasComponent<MaterialComponent>())
				{
					m_SelectionContext.AddComponent<MaterialComponent>();
				}
			}
			else
			{
				if (m_SelectionContext.HasComponent<MaterialComponent>())
				{
					m_SelectionContext.RemoveComponent<MaterialComponent>();
				}
			}

			ImGui::Checkbox("ReceivesPBR", &component.ReceivesPBR);
			ImGui::Checkbox("ReceivesIBL", &component.ReceivesIBL);
			ImGui::Checkbox("ReceivesLight", &component.ReceivesLight);
			ImGui::Checkbox("ReceivesShadow", &component.ReceivesShadow);
			ImGui::Checkbox("ProjectionShadow", &component.ProjectionShadow);

			// Utils::PbrUiRenderer(component.Material);
		});

		DrawComponent<ModelRendererComponent>("Model Renderer", entity, [this](auto &component)
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

						if (!component.ModelPath.empty() && component.Model != nullptr)
						{
							auto& hierarchy = m_SelectionContext.AddComponent<HierarchyComponent>();
							for (Ref<Mesh> &obj : component.Model->GetMeshs())
							{
								Entity subEntity = m_SelectionContext.CreateChild(obj->GetName());
								auto &mesh = subEntity.AddComponent<MeshRendererComponent>();
								mesh.Id = obj->GetID();
								hierarchy.Children.push_back(subEntity.GetUUID());
							}
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

			ImGui::Checkbox("FlipUV", &component.FlipUV);

			ImGui::Checkbox("ReceivesPBR", &component.ReceivesPBR);
			ImGui::Checkbox("ReceivesIBL", &component.ReceivesIBL);
			ImGui::Checkbox("ReceivesLight", &component.ReceivesLight);
			ImGui::Checkbox("ReceivesShadow", &component.ReceivesShadow);
			ImGui::Checkbox("ProjectionShadow", &component.ProjectionShadow);

			ImGui::Checkbox("GammaCorrection", &component.GammaCorrection);

			if (component.Model->GetIsAnimation())
			{
				ImGui::Checkbox("ReceivesAnimator", &component.ReceivesAnimator);

				auto animator = component.Model->GetAnimator();
				float currentPos = component.Model->GetAnimator()->GetCurrentTime();
				float totalPos = component.Model->GetAnimator()->GetDuration();

				float progress = (totalPos > 0.0f) ? (currentPos / totalPos) : 0.0f;

				if (ImGui::SliderFloat("Animation Progress", &progress, 0.0f, 1.0f))
				{
					animator->SetPaused(true);
					animator->SetProgress(progress);
				}

				// 播放/暂停控制按钮
				if (ImGui::Button(animator->GetPaused() ? "Play" : "Pause"))
				{
					animator->SetPaused(!animator->GetPaused());
				}

				// 显示进度文本
				ImGui::Text("Time: %.2f / %.2f", currentPos, totalPos);
				ImGui::ProgressBar(progress);
			}
			else
				ImGui::Text("static objects");
		});

		DrawComponent<MeshRendererComponent>("Mesh Renderer", entity, [this](auto &component)
		{
			ImGui::Checkbox("IsMaterial", &component.IsMaterial);
			if (component.IsMaterial)
			{
				if (!m_SelectionContext.HasComponent<MaterialComponent>())
				{
					m_SelectionContext.AddComponent<MaterialComponent>();
				}
			}
			else
			{
				if (m_SelectionContext.HasComponent<MaterialComponent>())
				{
					m_SelectionContext.RemoveComponent<MaterialComponent>();
				}
			} 
		});

		DrawComponent<MaterialComponent>("PBR Material", entity, [this](auto &component){ 
			ImGui::Text("PBR Material");

			{
				const char *buttonText = "Preview Material";
				ImVec2 textSize = ImGui::CalcTextSize(buttonText);
				float width = textSize.x + 10.0f;
				ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);
				ImGui::Checkbox("Preview Material", &m_ShowMaterialEditor);

				if (m_ShowMaterialEditor)
				{
					ImVec2 propsPos = ImGui::GetWindowPos();
					ImVec2 propsSize = ImGui::GetWindowSize();

					ImVec2 nextPos = ImVec2(propsPos.x + propsSize.x + 5.0f, propsPos.y);

					ImGui::SetNextWindowPos(nextPos, ImGuiCond_Always);
					ImGui::SetNextWindowSize(ImVec2(m_PreviewSize, m_PreviewSize), ImGuiCond_Always);

					ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;

					if (ImGui::Begin("Preview Material", &m_ShowMaterialEditor, flags))
					{
						float ts = ImGui::GetIO().DeltaTime;

						m_PreviewMaterialCamera.SetViewportSize(m_PreviewSize, m_PreviewSize);

						m_PreviewFramebuffer->Bind();
						Renderer::SetClearColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
						Renderer::Clear();
						m_PreviewMaterialCamera.OnPreviewMaterialUpdate(ts);
						Renderer3D::BeginScene(m_PreviewMaterialCamera);

						Renderer3D::DrawDirectionalLight(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.2f, 1.2f, 1.2f));
						glm::mat4 sphereTransform = glm::mat4(1.0f);
						sphereTransform *= glm::scale(glm::mat4(1.0f), glm::vec3(1.6f));
						Renderer3D::DrawSphere(sphereTransform, component, -1);

						Renderer3D::EndScene();
						m_PreviewFramebuffer->UnBind();

						uint32_t textureID = m_PreviewFramebuffer->GetColorAttachmentRendererID();
						ImVec2 viewportSize = ImGui::GetContentRegionAvail();
						ImGui::Image((void *)(uintptr_t)textureID, viewportSize, {0, 1}, {1, 0});
					}
					ImGui::End();
				}
			}

			ImGui::Separator();
			ImGui::Spacing();

			ImGui::ColorEdit3("Albedo", glm::value_ptr(component.Albedo));
			ImGui::DragFloat("Roughness", &component.Roughness, 0.1f, 0.0f, 1.0f);
			ImGui::DragFloat("Metallic", &component.Metallic, 0.1f, 0.0f, 1.0f);
			ImGui::DragFloat("AO", &component.Ao, 0.1f, 0.0f, 1.0f);
			ImGui::ColorEdit3("Emissive", glm::value_ptr(component.Emissive));

			std::vector<std::string> arr = {"Albedo", "Normal", "Metallic", "Roughness", "AO", "Emissive", "Height"};

			std::string *mapPointers[] = {
				&component.AlbedoMap,
				&component.NormalMap,
				&component.MetallicMap,
				&component.RoughnessMap,
				&component.AmbientOcclusionMap,
				&component.EmissiveMap,
				&component.HeightMap
			};
			ImGui::Text("Texture Maps");
			ImGui::Separator();

			float windowVisibleX2 = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
			float itemWidth = 100.0f;
			float itemSpacing = 10.0f;

			for (int i = 0; i < arr.size(); i++)
			{
				ImGui::PushID(i);
				ImGui::BeginGroup();

				std::string *currentMapPath = mapPointers[i];

				if (ImGui::Button("X", ImVec2(20, 20)))
				{
					*currentMapPath = "";
				}

				ImTextureID texID = !currentMapPath->empty() 
				? (ImTextureID)TextureManager::Get().GetTexture(*currentMapPath)->GetRendererID() 
				: (ImTextureID)m_DefaultTexture->GetRendererID();

				float itemWidth = 100.0f;
				float itemHeight = 100.0f;
				if (ImGui::ImageButton(("##" + arr[i]).c_str(), texID, ImVec2(itemWidth, itemHeight),
									   ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
				{
				}

				ImGui::Text(arr[i].c_str());
				ImGui::EndGroup();

				float lastItemX2 = ImGui::GetItemRectMax().x;
				float nextItemX2 = lastItemX2 + itemSpacing + itemWidth;
				if (i + 1 < arr.size() && nextItemX2 < windowVisibleX2)
					ImGui::SameLine(0.0f, itemSpacing);

				ImGui::PopID();
			}
			
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
