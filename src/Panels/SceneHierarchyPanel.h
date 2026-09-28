#pragma once

#include "src/Core/Base.h"
#include "src/Scene/Scene.h"
#include "src/Scene/Entity.h"
#include "src/Renderer/Framebuffer.h"
#include "src/Renderer/Renderer.h"

namespace Mc {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);

		void OnImGuiRender();

		Entity GetSelectedEntity() const { return m_SelectionContext; }
		void SetSelectedEntity(Entity entity);
	private:
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entry);

		bool IsParentVisible(Entity entity);

		void DrawEntityNode(Entity entity);
		void DrawComponents(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;

		Ref<Framebuffer> m_PreviewFramebuffer;
		EditorCamera m_PreviewMaterialCamera;
		Ref<Texture2D> m_DefaultTexture, m_VisibleIcon, m_VisibleOffIcon;
		
		bool m_ShowMaterialEditor;
		float m_PreviewSize;
	};
}
