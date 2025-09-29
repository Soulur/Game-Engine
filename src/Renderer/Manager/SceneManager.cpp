#include "SceneManager.h"

namespace Mc
{
    Scope<SceneManager> SceneManager::Create(entt::registry &registry)
    {
        return CreateScope<SceneManager>(registry);
    }
}