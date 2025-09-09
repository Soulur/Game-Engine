#pragma once

#include "src/Core/Base.h"

struct GLFWwindow;

namespace Mc
{

	class GraphicsContext
	{
	public:
		GraphicsContext(GLFWwindow *windowHandle);

		void Init();
		void SwapBuffers();

		static Scope<GraphicsContext> Create(void *window);
	private:
		GLFWwindow *m_WindowHandle;
	};

}