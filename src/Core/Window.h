#pragma once

#include "src/Renderer/GraphicsContext.h"
#include "src/Core/Base.h"
#include "src/Events/Event.h"

#include <GLFW/glfw3.h>

namespace Mc
{
	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;

		WindowProps(const std::string &title = "Engine",
					uint32_t width = 1280,
					uint32_t height = 720)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event &)>;

		Window(const WindowProps &props);
		virtual ~Window();

		void OnUpdate();

		unsigned int GetWidth() const { return m_Data.Width; }
		unsigned int GetHeight() const { return m_Data.Height; }

		// Window attributes
		void SetEventCallback(const EventCallbackFn &callback) { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled);
		bool IsVSync() const;

		virtual void *GetNativeWindow() const { return m_Window; }

		static Scope<Window> Create(const WindowProps &props = WindowProps());
	private:
		virtual void Init(const WindowProps &props);
		virtual void Shutdown();

	private:
		GLFWwindow *m_Window;
		Scope<GraphicsContext> m_Context;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}