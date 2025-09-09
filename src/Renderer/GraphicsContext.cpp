#include "GraphicsContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Mc
{

	GraphicsContext::GraphicsContext(GLFWwindow *windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}

	void GraphicsContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

		LOG_CORE_INFO("OpenGL Info:");
		LOG_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		LOG_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		LOG_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));
	}

	void GraphicsContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

	Scope<GraphicsContext> GraphicsContext::Create(void *window)
	{
		return CreateScope<GraphicsContext>(static_cast<GLFWwindow *>(window));
	}
}