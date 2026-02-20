#include "EditorLayer.h"
#include "src/Core/Input.h"
#include "src/Core/Application.h"
#include "src/Renderer/Renderer.h"
#include "src/Renderer/Renderer3D.h"
#include "src/Utils/PlatformUtils.h"
#include "src/Scene/SceneSerializer.h"

#include "src/Math/Math.h"

#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>


namespace Mc
{
    extern const std::filesystem::path g_AssetPath;

    ImGuizmo::OPERATION m_CurrentOperation = ImGuizmo::TRANSLATE;

    EditorLayer::EditorLayer()
        : Layer("EditorLayer")
    //  m_RunCameraController((float)Application::Get().GetWindow().GetWidth() / (float)Application::Get().GetWindow().GetHeight())
    {
    }

    void EditorLayer::OnAttach()
    {
        m_IconMinimize = Texture2D::Create("Resources/Icons/minus.png");
        m_IconMaximize = Texture2D::Create("Resources/Icons/checkbox-blank-outline.png");
        m_IconRestore = Texture2D::Create("Resources/Icons/checkbox-multiple-blank-outline.png");
        m_IconClose = Texture2D::Create("Resources/Icons/close.png");

        m_IconPlay = Texture2D::Create("Resources/Icons/PlayButton.png");
        m_IconPause = Texture2D::Create("Resources/Icons/PauseButton.png");
        m_IconSimulate = Texture2D::Create("Resources/Icons/SimulateButton.png");
        m_IconStep = Texture2D::Create("Resources/Icons/StepButton.png");
        m_IconStop = Texture2D::Create("Resources/Icons/StopButton.png");

        FramebufferSpecification fbSpec;
        fbSpec.Attachments = {FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::DEPTH24STENCIL8};
        fbSpec.Width = Application::Get().GetWindow().GetWidth();
        fbSpec.Height = Application::Get().GetWindow().GetHeight();

        m_Framebuffer = Framebuffer::Create(fbSpec);
        m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 100.0f);

        // m_GameFramebuffer = Framebuffer::Create(fbSpec);

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
    }

    void EditorLayer::OnDetach()
    {
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        
        // Edit
        if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
            m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f && // zero sized framebuffer is invalid
            (spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
        {
            m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
        }

        Renderer3D::ResetStats();
        m_Framebuffer->Bind();
        Renderer::SetClearColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        Renderer::Clear();

        // Clear our entity ID attachment to -1
        m_Framebuffer->ClearAttachment(1, -1);

        
        switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				// if (m_ViewportFocused)
				// 	m_CameraController.OnUpdate(ts);

				m_EditorCamera.OnUpdate(ts);

				m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
				break;
			}
			case SceneState::Simulate:
			{
				m_EditorCamera.OnUpdate(ts);

				m_ActiveScene->OnUpdateSimulation(ts, m_EditorCamera);
				break;
			}
			case SceneState::Play:
			{
				m_ActiveScene->OnUpdateRuntime(ts);
				break;
			}
		}

        {
            auto [mx, my] = ImGui::GetMousePos();
            mx -= m_ViewportBounds[0].x;
            my -= m_ViewportBounds[0].y;
            glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
            my = viewportSize.y - my;
            int mouseX = (int)mx;
            int mouseY = (int)my;

            if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
            {
                int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
                m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, m_ActiveScene.get());
            }
        }

        // Renderer3D::BeginScene(m_EditCameraController.GetCamera());
        OnOverlayRender();
        m_Framebuffer->UnBind();
    }

    void EditorLayer::OnImGuiRender()
    {
        UI_MainMenuBar();

        // Note: Switch this to true to enable dockspace
        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + 45.0f));
            ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - 45.0f));
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        // -------------------------------------------------------------------------------------------------------------------------


        m_SceneHierarchyPanel.OnImGuiRender();
        m_ProjectBrowserPanel.OnImGuiRender();


        // ---------------------------------------------------------------------------------------


        // ---------------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------------
        // Log
        // ImGui::Begin("LOG");

        // std::string result;
        // std::ifstream in("log.log", std::ios::in | std::ios::binary);
        // if (in)
        // {
        //     in.seekg(0, std::ios::end);
        //     size_t size = in.tellg();
        //     if (size != -1)
        //     {
        //         result.resize(size);
        //         in.seekg(0, std::ios::beg);
        //         in.read(&result[0], size);
        //         in.close();
        //     }
        // }

        // ImGui::Text(result.c_str());
        // ImGui::End();
        // ---------------------------------------------------------------------------------------------------------

        /*
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::Begin("Stats");
        ImGui::PopStyleColor();
        std::string name = "None";
        if (m_HoveredEntity)
        {
            name = m_HoveredEntity.GetComponent<TagComponent>().Tag;
        }
        ImGui::Text("Hovered Entity: %s", name.c_str());

        auto stats = Renderer3D::GetStats();
        ImGui::Text("Renderer2D Stats:");
        ImGui::Text("Draw Calls: %d", stats.DrawCalls);
        ImGui::Text("Sphere: %d", stats.SphereCount / 2);
        ImGui::Text("Model: %d", stats.ModelCount / 2);

        ImGui::End();
        */

        // ---------------------------------------------------------------------------------------------------------
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin("Edit", nullptr, ImGuiWindowFlags_NoScrollWithMouse);
        {
            auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
            auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
            auto viewportOffset = ImGui::GetWindowPos();
            m_ViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
            m_ViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};

            m_ViewportFocused = ImGui::IsWindowFocused();
            m_ViewportHovered = ImGui::IsWindowHovered();

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                m_SceneState = SceneState::Edit;
                Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused && !m_ViewportHovered);
            }
        }

        m_ViewportSize = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};
        ImGui::Image(reinterpret_cast<void *>(m_Framebuffer->GetColorAttachmentRendererID()), ImVec2{m_ViewportSize.x, m_ViewportSize.y}, ImVec2{0, 1}, ImVec2{1, 0});

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("FOLDER_PANEL"))
            {
                const wchar_t *path = (const wchar_t *)payload->Data;
                auto scenepath = std::filesystem::path(g_AssetPath) / path;
                if (scenepath.extension() == ".scene")
                    OpenScene(scenepath);
                else
                    LOG_CORE_WARN("not .scene");
            }
            ImGui::EndDragDropTarget();
        }

        // Gizmos
        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity && m_GizmoType != -1)
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();

            ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y,
                              m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);

            // Camera

            // Runtime camera from entity
            // auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
            // const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
            // const glm::mat4& cameraProjection = camera.GetProjection();
            // glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());

            // Editor camera
            const glm::mat4 &cameraProjection = m_EditorCamera.GetProjection();
            glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

            // Entity transform
            auto &tc = selectedEntity.GetComponent<TransformComponent>();
            glm::mat4 transform = tc.GetTransform();

            // Snapping
            bool snap = Input::IsKeyPressed(Key::LeftControl);
            float snapValue = 0.5f; // Snap to 0.5m for translation/scale
            // Snap to 45 degrees for rotation
            if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.0f;

            float snapValues[3] = {snapValue, snapValue, snapValue};

            ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                 (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
                                 nullptr, snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 translation, rotation, scale;
                Math::DecomposeTransform(transform, translation, rotation, scale);

                glm::vec3 deltaRotation = rotation - tc.Rotation;
                tc.Translation = translation;
                tc.Rotation += deltaRotation;
                tc.Scale = scale;
            }
        }

        UI_Toolbar();

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::End();
    }

    void EditorLayer::OnEvent(Event &e)
    {
        if (m_SceneState == SceneState::Edit && m_ViewportHovered)
            m_EditorCamera.OnEvent(e);

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent &e)
    {
        // Shortcuts
        if (e.IsRepeat())
            return false;

        bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

        switch (e.GetKeyCode())
        {
            case Key::N:
            {
                if (control)
                    NewScene();
                break;
            }
            case Key::O:
            {
                if (control)
                    OpenScene();

                break;
            }
            case Key::S:
            {
                if (control)
                {
                    if (shift)
                        SaveSceneAs();
                    else
                        SaveScene();
                }

                break;
            }
            case Key::D:
            {
                if (control)
                    OnDuplicateEntity();

                break;
            }

            // Gizmos
            case Key::Q:
            {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = -1;
                break;
            }
            case Key::W:
            {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                break;
            }
            case Key::E:
            {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                break;
            }
            case Key::R:
            {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::SCALE;
                break;
            }
        }
        return true;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent &e)
    {
        if (e.GetMouseButton() == Mouse::ButtonLeft)
        {
            if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
                m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
        }
        return false;
    }

    void EditorLayer::OnOverlayRender()
    {
        Renderer3D::BeginScene(m_EditorCamera);

        // 画线
        // if (Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity())
        // {
        //     const TransformComponent &transform = selectedEntity.GetComponent<TransformComponent>();

        //     Renderer3D::DrawSphere(transform.GetTransform(), glm::vec4(1.0f, 0.5f, 1.0f, 1.0f));
        // }
        
        Renderer3D::EndScene();
    }

    void EditorLayer::NewScene()
    {
        m_EditorScene = CreateRef<Scene>();
        m_SceneHierarchyPanel.SetContext(m_EditorScene);

        m_EditorScene->NewScene();
        m_ActiveScene = m_EditorScene;
        m_EditorScenePath = std::filesystem::path();
    }

    void EditorLayer::OpenScene()
    {
        std::string filepath = FileDialogs::OpenFile("Scene (*.scene)\0*.scene\0");
        if (!filepath.empty())
            OpenScene(filepath);
    }

    void EditorLayer::OpenScene(const std::filesystem::path &path)
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        Ref<Scene> newScene = CreateRef<Scene>();
        SceneSerializer serializer(newScene);
        if (serializer.Deserialize(path.string()))
        {
            m_EditorScene = newScene;
            m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_SceneHierarchyPanel.SetContext(m_EditorScene);

            m_ActiveScene = m_EditorScene;
            m_EditorScenePath = path;
        }
    }

    void EditorLayer::SaveScene()
    {
        if (!m_EditorScenePath.empty())
            SeializeScene(m_ActiveScene, m_EditorScenePath.string());
        else
            SaveSceneAs();
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string filepath = FileDialogs::SaveFile("Scene (*.scene)\0*.scene\0");
        if (!filepath.empty())
        {
            SeializeScene(m_ActiveScene, filepath);
            m_EditorScenePath = filepath;
        }
    }

    void EditorLayer::SeializeScene(Ref<Scene> scene, const std::string &path)
    {
        SceneSerializer serializer(scene);
        serializer.Serialize(path);
    }

    void EditorLayer::OnScenePlay()
    {
        if (m_SceneState == SceneState::Simulate)
            OnSceneStop();

        m_SceneState = SceneState::Play;

        m_ActiveScene = Scene::Copy(m_EditorScene);
        m_ActiveScene->OnRuntimeStart();

		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OnSceneSimulate()
    {
        if (m_SceneState == SceneState::Play)
            OnSceneStop();

        m_SceneState = SceneState::Simulate;

        m_ActiveScene = Scene::Copy(m_EditorScene);
        m_ActiveScene->OnSimulationStart();

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }
    void EditorLayer::OnSceneStop()
    {
        if (m_SceneState == SceneState::Play)
            m_ActiveScene->OnRuntimeStop();
        else if (m_SceneState == SceneState::Simulate)
            m_ActiveScene->OnSimulationStop();

        m_SceneState = SceneState::Edit;

        m_ActiveScene = m_EditorScene;
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OnScenePause()
    {
        if (m_SceneState == SceneState::Edit)
            return;

        m_ActiveScene->SetPaused(true);
    }

    void EditorLayer::UI_MainMenuBar()
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        float topBarHeight = 45.0f;

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

        if (ImGui::BeginViewportSideBar("##MainMenuBar", viewport, ImGuiDir_Up, topBarHeight, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking))
        {
            ImU32 colLeft = IM_COL32(28, 92, 47, 255);
            ImU32 colRight = IM_COL32(28, 28, 28, 255);
            {
                ImDrawList *drawList = ImGui::GetWindowDrawList();
                ImVec2 minP = ImGui::GetWindowPos();
                ImVec2 maxP = ImVec2(minP.x + viewport->Size.x / 4.0f, minP.y + topBarHeight);
                drawList->AddRectFilledMultiColor(minP, maxP, colLeft, colRight, colRight, colLeft);
            }

            {
                ImGui::SameLine(0, 20.0f);

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 13.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f)); // 文字居中

                const char *MenuFile = "File";
                float textWidth = ImGui::CalcTextSize(MenuFile).x;
                float itemWidth = textWidth + 20.0f;
                float itemHeight = topBarHeight * 0.3f;

                ImVec2 screenPos = ImGui::GetCursorScreenPos();

                // ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.12f, 0.45f, 1.0f, 1.0f));
                // ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.12f, 0.45f, 1.0f, 1.0f));

                ImGui::Selectable(MenuFile, false, ImGuiSelectableFlags_None, ImVec2(itemWidth, itemHeight));

                if (ImGui::IsItemHovered())
                    ImGui::OpenPopup(MenuFile);

                // ImGui::PopStyleColor(2);
                ImGui::PopStyleVar(2);

                ImGui::SetNextWindowPos(ImVec2(screenPos.x, screenPos.y + itemHeight + 3.0f));

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);

                if (ImGui::BeginPopup(MenuFile))
                {
                    if (ImGui::MenuItem("New", "Ctrl+N"))
                        NewScene();

                    if (ImGui::MenuItem("Open...", "Ctrl+O"))
                        OpenScene();

                    if (ImGui::MenuItem("Save", "Ctrl+S"))
                        SaveScene();

                    if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                        SaveSceneAs();

                    if (ImGui::MenuItem("Exit")) Application::Get().Close();
                    ImGui::EndPopup();
                }
                ImGui::PopStyleVar(3);
                ImGui::SameLine(0, 0);
            }

            auto *window = static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow());

            {
                float itemWidth = 20.0f;
                float itemHeight = topBarHeight * 0.4f;

                float offset = 5.0f;
                float buttonsAreaWidth = (itemWidth + offset * 2) * 3;

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(offset, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f)); // 点击时微亮

                ImGui::SameLine(ImGui::GetWindowWidth() - buttonsAreaWidth);

                ImTextureID minIconID = (ImTextureID)m_IconMinimize->GetRendererID();
                if (ImGui::ImageButton("##Minimize", minIconID, ImVec2(itemWidth, itemHeight),
                                       ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
                {
                    glfwIconifyWindow(window);
                }

                ImGui::SameLine(0, 0);

                ImTextureID maxIcon = glfwGetWindowAttrib(window, GLFW_MAXIMIZED) ? (ImTextureID)m_IconRestore->GetRendererID() : (ImTextureID)m_IconMaximize->GetRendererID();
                if (ImGui::ImageButton("##Maximize", maxIcon, ImVec2(itemWidth, itemHeight), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0)))
                {
                    if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
                        glfwRestoreWindow(window);
                    else
                        glfwMaximizeWindow(window);
                }

                ImGui::SameLine(0, 0);

                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
                if (ImGui::ImageButton("##Close", (ImTextureID)m_IconClose->GetRendererID(), ImVec2(itemWidth, itemHeight), ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0)))
                {
                    Application::Get().Close();
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);
            }

            {
                if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
                {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
                            glfwRestoreWindow(window);
                        else
                            glfwMaximizeWindow(window);
                    }
                }

                if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
                {
                    if (!glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
                    {
                        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);

                        if (delta.x != 0.0f || delta.y != 0.0f)
                        {
                            int winX, winY;
                            glfwGetWindowPos(window, &winX, &winY);
                            glfwSetWindowPos(window, winX + (int)delta.x, winY + (int)delta.y);

                            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                        }
                    }
                }
            }

            ImGui::End();
        }

        ImGui::PopStyleColor();
    }

    void EditorLayer::UI_Toolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto &colors = ImGui::GetStyle().Colors;
        const auto &buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto &buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.6f));

        float toolbarHeight = 30.0f;
        float toolbarWidth = 120.0f;

        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();

        float targetPosX = windowPos.x + (windowSize.x - toolbarWidth) * 0.5f;
        float targetPosY = windowPos.y + 5.0f;

        ImGui::SetNextWindowPos(ImVec2(targetPosX, targetPosY));

        ImGui::SetNextWindowSize(ImVec2(toolbarWidth, toolbarHeight));

        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoDecoration |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("##toolbar", nullptr, toolbarFlags);

        bool toolbarEnabled = (bool)m_ActiveScene;

        ImVec4 bgColor = ImVec4(0, 0, 0, 0); // 背景色，通常透明
        ImVec4 tintColor = ImVec4(1, 1, 1, 1);
        if (!toolbarEnabled)
            tintColor.w = 0.5f;

        float size = toolbarHeight - 10.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - size * 2.5f);

        bool hasPlayButton = true;
        bool hasSimulateButton = true;
        bool hasPauseButton = true;

        if (hasPlayButton)
        {
            Ref<Texture2D> icon = (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate) ? m_IconPlay : m_IconStop;
            const char *buttonID = (m_SceneState == SceneState::Play) ? "##EditButton" : "##PlayButton";
            ImTextureID textureID = (ImTextureID)icon->GetRendererID();

            if (ImGui::ImageButton(buttonID, (ImTextureRef)textureID, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), bgColor, tintColor))
            {
                if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)
                    OnScenePlay();
                else if (m_SceneState == SceneState::Play)
                    OnSceneStop();
            }
        }

        if (hasSimulateButton)
        {            
            ImGui::SameLine();

            Ref<Texture2D> icon = (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play) ? m_IconSimulate : m_IconStop;
            const char *buttonID = (m_SceneState == SceneState::Simulate) ? "##EditButton" : "##SimulateButton";
            ImTextureID textureID = (ImTextureID)icon->GetRendererID();

            if (ImGui::ImageButton(buttonID, (ImTextureRef)textureID, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), bgColor, tintColor))
            {
                if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play)
                    OnSceneSimulate();
                else if (m_SceneState == SceneState::Simulate)
                    OnSceneStop();
            }
        }

        if (hasPauseButton)
        {
            ImGui::SameLine();

            Ref<Texture2D> icon = m_ActiveScene->IsPaused() ? m_IconStep : m_IconPause;
            const char *buttonID = m_ActiveScene->IsPaused() ? "##StepButton" : "##PauseButton";
            ImTextureID textureID = (ImTextureID)icon->GetRendererID();

            if (ImGui::ImageButton(buttonID, (ImTextureRef)textureID, ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), bgColor, tintColor))
            {
                if (!m_ActiveScene->IsPaused())
                    OnScenePause();
                else
                    m_ActiveScene->SetPaused(false);
            }
        }

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(4);
        ImGui::End();
    }

    void EditorLayer::OnDuplicateEntity()
    {
        if (m_SceneState != SceneState::Edit)
            return;        

        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();

        if (selectedEntity)
        {
            m_EditorScene->DuplicateEntity(selectedEntity);
        }
    }
}