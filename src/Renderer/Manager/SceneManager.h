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
            
            registry.on_construct<ShadowComponent>().connect<&SceneManager::OnShadowCreated>(this);
            registry.on_destroy<ShadowComponent>().connect<&SceneManager::OnShadowDestroyed>(this);

            // registry.on_construct<MaterialComponent>().connect<&SceneManager::OnMaterialCreated>(this);
            // registry.on_destroy<MaterialComponent>().connect<&SceneManager::OnMaterialDestroyed>(this);
        }

        static Scope<SceneManager> Create(entt::registry &registry);
    private:
        void OnShadowCreated(entt::registry &registry, entt::entity entity)
        {
            if (registry.any_of<PointLightComponent>(entity))
            {
                auto& light = registry.get<PointLightComponent>(entity);
                auto &shadow = registry.get<ShadowComponent>(entity);
                Renderer3D::AddPointShadow(entity, shadow.Resolution);
                LOG_CORE_ERROR("1");
            }

            if (registry.any_of<DirectionalLightComponent>(entity))
            {
                auto &light = registry.get<DirectionalLightComponent>(entity);
                auto &shadow = registry.get<ShadowComponent>(entity);
                Renderer3D::AddDirectionalShadow(entity, shadow.Resolution);
                LOG_CORE_ERROR("2");
            }

            if (registry.any_of<SpotLightComponent>(entity))
            {
                auto &light = registry.get<SpotLightComponent>(entity);
                auto &shadow = registry.get<ShadowComponent>(entity);
                Renderer3D::AddSpotShadow(entity, shadow.Resolution);
                LOG_CORE_ERROR("3");
            }
        }

        void OnShadowDestroyed(entt::registry &registry, entt::entity entity)
        {
            if (registry.any_of<PointLightComponent>(entity))
            {
                auto &light = registry.get<PointLightComponent>(entity);
                auto &shadow = registry.get<ShadowComponent>(entity);
                Renderer3D::DelPointShadow(entity);
                LOG_CORE_ERROR("0");
            }

            if (registry.any_of<DirectionalLightComponent>(entity))
            {
                auto &light = registry.get<DirectionalLightComponent>(entity);
                auto &shadow = registry.get<ShadowComponent>(entity);
                Renderer3D::DelDirectionalShadow(entity);
                LOG_CORE_ERROR("-1");
            }

            if (registry.any_of<SpotLightComponent>(entity))
            {
                auto &light = registry.get<SpotLightComponent>(entity);
                auto &shadow = registry.get<ShadowComponent>(entity);
                Renderer3D::DelSpotShadow(entity);
                LOG_CORE_ERROR("-2");
            }
        }

        // void OnMaterialCreated(entt::registry &registry, entt::entity entity)
        // {

        // }

        // void OnMaterialDestroyed(entt::registry &registry, entt::entity entity)
        // {

        // }
    };
}