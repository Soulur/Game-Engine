#pragma once


#include "src/Core/Layer.h"
#include "src/Core/Base.h"

#include "src/Events/KeyEvent.h"
// #include "src/Events/Event.h"

#include "src/Renderer/OrthographicCameraController.h"
#include "src/Renderer/EditorCamera.h"
#include "src/Renderer/Framebuffer.h"

#include "src/Panels/SceneHierarchyPanel.h"
#include "src/Panels/ProjectBrowserPanel.h"

#include <filesystem>

namespace Mc
{

    class EditorLayer : public Layer
    {
    public:
        EditorLayer();
        virtual ~EditorLayer() = default;

        virtual void OnAttach() override;
        virtual void OnDetach() override;

        void OnUpdate(Timestep ts) override;
        virtual void OnImGuiRender() override;
        void OnEvent(Event &e) override;
    private:
        bool OnKeyPressed(KeyPressedEvent &e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent &e);
        void OnOverlayRender();

        void NewScene();
        void OpenScene();
        void OpenScene(const std::filesystem::path &path);
        void SaveScene();
        void SaveSceneAs();

        void SeializeScene(Ref<Scene> scene, const std::string &path);

    private:
        EditorCamera m_EditorCamera;

        Ref<Framebuffer> m_EditorFramebuffer;
        bool m_EditorViewportFocused = false;
        bool m_EditorViewportHovered = false;
        glm::vec2 m_EditorViewportSize = {0.0f, 0.0f};
        glm::vec2 m_EditorViewportBounds[2];

        int m_GizmoType = -1;
        
        Entity m_HoveredEntity;

        // --------------------------------------------------

        Ref<Framebuffer> m_GameFramebuffer;
        bool m_GameViewportFocused = false;
        bool m_GameViewportHovered = false;
        glm::vec2 m_GameViewportSize = {0.0f, 0.0f};
        glm::vec2 m_GameViewportBounds[2];

        // --------------------------------------------------

        enum class SceneState
        {
            Edit = 0,
            Game = 1,
            Simulate = 2
        };

        SceneState m_SceneState = SceneState::Edit;

        std::filesystem::path m_EditorScenePath;
        Ref<Scene> m_ActiveScene, m_EditorScene;

        // Panels
        SceneHierarchyPanel m_SceneHierarchyPanel;
        ProjectBrowserPanel m_ProjectBrowserPanel;
    };
}
