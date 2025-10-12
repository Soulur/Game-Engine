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
        FramebufferSpecification fbSpec;
        fbSpec.Attachments = {FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::DEPTH24STENCIL8};
        fbSpec.Width = Application::Get().GetWindow().GetWidth();
        fbSpec.Height = Application::Get().GetWindow().GetHeight();

        m_EditorFramebuffer = Framebuffer::Create(fbSpec);
        m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 100.0f);

        m_GameFramebuffer = Framebuffer::Create(fbSpec);

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
    }

    void EditorLayer::OnDetach()
    {
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        // Edit
        if (FramebufferSpecification spec = m_EditorFramebuffer->GetSpecification();
            m_EditorViewportSize.x > 0.0f && m_EditorViewportSize.y > 0.0f && // zero sized framebuffer is invalid
            (spec.Width != m_EditorViewportSize.x || spec.Height != m_EditorViewportSize.y))
        {
            m_EditorFramebuffer->Resize((uint32_t)m_EditorViewportSize.x, (uint32_t)m_EditorViewportSize.y);
            m_EditorCamera.SetViewportSize(m_EditorViewportSize.x, m_EditorViewportSize.y);
        }

        Renderer3D::ResetStats();
        m_EditorFramebuffer->Bind();
        Renderer::SetClearColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        Renderer::Clear();

        // Clear our entity ID attachment to -1
        m_EditorFramebuffer->ClearAttachment(1, -1);

        if (m_EditorViewportFocused && m_EditorViewportHovered)
            m_EditorCamera.OnUpdate(ts);

        m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);

        {
            auto [mx, my] = ImGui::GetMousePos();
            mx -= m_EditorViewportBounds[0].x;
            my -= m_EditorViewportBounds[0].y;
            glm::vec2 viewportSize = m_EditorViewportBounds[1] - m_EditorViewportBounds[0];
            my = viewportSize.y - my;
            int mouseX = (int)mx;
            int mouseY = (int)my;

            if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
            {
                int pixelData = m_EditorFramebuffer->ReadPixel(1, mouseX, mouseY);
                m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, m_ActiveScene.get());
            }
        }

        // Renderer3D::BeginScene(m_EditCameraController.GetCamera());
        OnOverlayRender();
        m_EditorFramebuffer->UnBind();

        // // Game
        // ------------------------------------------------------------------------------------
        if (FramebufferSpecification spec = m_GameFramebuffer->GetSpecification();
            m_GameViewportSize.x > 0.0f && m_GameViewportSize.y > 0.0f && // zero sized framebuffer is invalid
            (spec.Width != m_GameViewportSize.x || spec.Height != m_GameViewportSize.y))
        {
            m_GameFramebuffer->Resize((uint32_t)m_GameViewportSize.x, (uint32_t)m_GameViewportSize.y);
            m_ActiveScene->OnViewportResize((uint32_t)m_GameViewportSize.x, (uint32_t)m_GameViewportSize.y);
        }

        m_GameFramebuffer->Bind();
        Renderer::SetClearColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
        Renderer::Clear();
        m_ActiveScene->OnUpdateRuntime(ts);
        m_GameFramebuffer->UnBind();
    }

    void EditorLayer::OnImGuiRender()
    {
        // Note: Switch this to true to enable dockspace
        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
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

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                // ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);1
                if (ImGui::MenuItem("New", "Ctrl+N"))
                    NewScene();

                if (ImGui::MenuItem("Open...", "Ctrl+O"))
                    OpenScene();

                if (ImGui::MenuItem("Save", "Ctrl+S"))
                    SaveScene();

                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                    SaveSceneAs();

                if (ImGui::MenuItem("Exit"))
                    Application::Get().Close();
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        m_SceneHierarchyPanel.OnImGuiRender();
        m_ProjectBrowserPanel.OnImGuiRender();


        // ---------------------------------------------------------------------------------------


        // ---------------------------------------------------------------------------------------

        // ---------------------------------------------------------------------------------------
        // Log
        ImGui::Begin("LOG");

        std::string result;
        std::ifstream in("log.log", std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1)
            {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
                in.close();
            }
        }

        ImGui::Text(result.c_str());
        ImGui::End();
        // ---------------------------------------------------------------------------------------------------------

        ImGui::Begin("Stats");
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

        // ---------------------------------------------------------------------------------------------------------
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin("Edit", nullptr, ImGuiWindowFlags_NoScrollWithMouse);
        {
            auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
            auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
            auto viewportOffset = ImGui::GetWindowPos();
            m_EditorViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
            m_EditorViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};

            m_EditorViewportFocused = ImGui::IsWindowFocused();
            m_EditorViewportHovered = ImGui::IsWindowHovered();

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                m_SceneState = SceneState::Edit;
                Application::Get().GetImGuiLayer()->BlockEvents(!m_EditorViewportFocused && !m_EditorViewportHovered);
            }
        }

        m_EditorViewportSize = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};
        ImGui::Image(reinterpret_cast<void *>(m_EditorFramebuffer->GetColorAttachmentRendererID()), ImVec2{m_EditorViewportSize.x, m_EditorViewportSize.y}, ImVec2{0, 1}, ImVec2{1, 0});

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

            ImGuizmo::SetRect(m_EditorViewportBounds[0].x, m_EditorViewportBounds[0].y,
                              m_EditorViewportBounds[1].x - m_EditorViewportBounds[0].x, m_EditorViewportBounds[1].y - m_EditorViewportBounds[0].y);

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

        ImGui::End();
        ImGui::PopStyleVar();

        // ------------------------------------------------------------------------------------------------
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin("Game");
        {
            auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
            auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
            auto viewportOffset = ImGui::GetWindowPos();
            m_GameViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
            m_GameViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};

            m_GameViewportFocused = ImGui::IsWindowFocused();
            m_GameViewportHovered = ImGui::IsWindowHovered();
        }

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        {
            m_SceneState = SceneState::Game;
            Application::Get().GetImGuiLayer()->BlockEvents(!m_GameViewportFocused && !m_GameViewportHovered);
        }
        m_GameViewportSize = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};
        ImGui::Image(reinterpret_cast<void *>(m_GameFramebuffer->GetColorAttachmentRendererID()), ImVec2{m_GameViewportSize.x, m_GameViewportSize.y}, ImVec2{0, 1}, ImVec2{1, 0});
        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::End();
    }

    void EditorLayer::OnEvent(Event &e)
    {
        if (m_SceneState == SceneState::Edit && m_EditorViewportHovered)
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
            if (m_EditorViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
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
        m_ActiveScene = CreateRef<Scene>();
        m_ActiveScene->OnViewportResize((uint32_t)m_GameViewportSize.x, (uint32_t)m_GameViewportSize.y);
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

        m_ActiveScene->NewScene();
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
        // if (m_SceneState != SceneState::Edit)
        //     OnSceneStop();

        Ref<Scene> newScene = CreateRef<Scene>();
        SceneSerializer serializer(newScene);
        if (serializer.Deserialize(path.string()))
        {
            m_EditorScene = newScene;
            m_EditorScene->OnViewportResize((uint32_t)m_EditorViewportSize.x, (uint32_t)m_EditorViewportSize.y);
            m_SceneHierarchyPanel.SetContext(m_EditorScene);

            m_ActiveScene = m_EditorScene;
            m_ActiveScene->OnViewportResize((uint32_t)m_GameViewportSize.x, (uint32_t)m_GameViewportSize.y);
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
}