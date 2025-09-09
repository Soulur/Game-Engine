#pragma once

#include "src/Core/Base.h"
#include "src/Renderer/VertexArray.h"

namespace Mc
{
	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();
		
		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
		static void SetClearColor(const glm::vec4 &color);
		static void Clear();

		static void DrawIndexed(const Ref<VertexArray> &vertexArray, uint32_t indexCount = 0);
	};
}