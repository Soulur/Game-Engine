#pragma once

#include "src/Core/Base.h"

#include "src/Renderer/Renderer3D.h"
#include "src/Scene/Components.h"

#include <entt/entt.hpp>

namespace Mc
{
    class SceneManager
    {
    public:;
        SceneManager(entt::registry &registry)
        {
            
            registry.on_construct<ShadowComponent>().connect<&SceneManager::OnPointShadowCreated>(this);
            registry.on_destroy<ShadowComponent>().connect<&SceneManager::OnPointShadowDestroyed>(this);
        }

        static Scope<SceneManager> Create(entt::registry &registry);
    private:
        void OnPointShadowCreated(entt::registry &registry, entt::entity entity)
        {
            if (registry.any_of<PointLightComponent>(entity))
            {
                auto& light = registry.get<PointLightComponent>(entity);
                auto &shadow = registry.get<ShadowComponent>(entity);
                Renderer3D::AddPointShadow(entity, shadow.Resolution);
                LOG_CORE_ERROR("1");
            }
        }

        void OnPointShadowDestroyed(entt::registry &registry, entt::entity entity)
        {
            if (registry.any_of<PointLightComponent>(entity))
            {
                auto &light = registry.get<PointLightComponent>(entity);
                auto &shadow = registry.get<ShadowComponent>(entity);
                Renderer3D::DelPointShadow(entity);
                LOG_CORE_ERROR("0");
            }
        }
    };
}