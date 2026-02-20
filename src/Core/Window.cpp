
#include "Window.h"

#include "src/Core/Input.h"

#include "src/Events/ApplicationEvent.h"
#include "src/Events/MouseEvent.h"
#include "src/Events/KeyEvent.h"
#include "src/Core/Log.h"

#include <glad/glad.h>

namespace Mc
{

	static uint8_t s_GLFWWindowCount = 0;

	Window::Window(const WindowProps &props)
	{
		Init(props);
	}

	Window::~Window()
	{
		Shutdown();
	}

	void Window::Init(const WindowProps &props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		LOG_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (s_GLFWWindowCount == 0)
		{
			int success = glfwInit();
		}

		{
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
			glfwSetWindowPos(m_Window, 100, 100);
			++s_GLFWWindowCount;
		}

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow *window, int width, int height)
								  {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event); });

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow *window)
								   {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event); });

		glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
						   {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, false);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, true);
					data.EventCallback(event);
					break;
				}
			} });

		glfwSetCharCallback(m_Window, [](GLFWwindow *window, unsigned int keycode)
							{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event); });

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow *window, int button, int action, int mods)
								   {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
			} });

		glfwSetScrollCallback(m_Window, [](GLFWwindow *window, double xOffset, double yOffset)
							  {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event); });

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow *window, double xPos, double yPos)
								 {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event); });
	}

	void Window::Shutdown()
	{

		glfwDestroyWindow(m_Window);
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void Window::OnUpdate()
	{

		glfwPollEvents();
		m_Context->SwapBuffers();
	}

	void Window::SetVSync(bool enabled)
	{

		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool Window::IsVSync() const
	{
		return m_Data.VSync;
	}

	Scope<Window> Window::Create(const WindowProps &props)
	{
		return CreateScope<Window>(props);
	}
}