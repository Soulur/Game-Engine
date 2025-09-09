#include "src/Core/Application.h"
#include "src/Core/Log.h"
#include "src/Core/Input.h"
#include "src/Core/Timer.h"
#include "src/Renderer/Renderer.h"
#include "src/Renderer/Renderer3D.h"

#include "src/Renderer/Manager/TextureManager.h"
#include "src/Renderer/Manager/ModelManager.h"
#include "src/Renderer/Manager/MeshManager.h"
#include "src/Renderer/Manager/MaterialManager.h"
// #include "src/Scripting/ScriptEngine.h"
#include "src/Utils/PlatformUtils.h"
#include "src/Core/Window.h"

namespace Mc {

	Application* Application::s_Instance = nullptr;

		Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		s_Instance = this;

		m_Window = Window::Create(WindowProps(specification.Name));
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

		Renderer::Init();
		Renderer3D::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	Application::~Application()
	{
		Renderer::Shutdown();
		Renderer3D::Shutdown();

		TextureManager::Get().Shutdown();
		MaterialManager::Get().Shutdown();
		ModelManager::Get().Shutdown();
		MeshManager::Get().Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled) 
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		while (m_Running)
		{
			float time = Time::GetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				{
					for (Layer* layer : m_LayerStack)	
						layer->OnUpdate(timestep);
				}

				m_ImGuiLayer->Begin();
				{
					for (Layer* layer : m_LayerStack)
						layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();
			}

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		Renderer::SetViewport(0, 0, e.GetWidth(), e.GetHeight());

		return false;
	}
}
