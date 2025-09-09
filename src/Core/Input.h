#pragma once

#include "src/Events/KeyCodes.h"
#include "src/Events/MouseCodes.h"

#include <glm/glm.hpp>

namespace Mc {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();

	};
}