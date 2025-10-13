#include "Scene.h"
#include "Entity.h"

#include "Components.h"
#include "src/Renderer/Renderer3D.h"
#include "src/Renderer/Manager/ModelManager.h"
// #include "ScriptableEntity.h"
// #include "src/Scripting/ScriptEngine.h"

#include <glm/glm.hpp>

namespace Mc 
{
	Scene::Scene()
	{
		m_CameraTexture = Texture2D::Create("Assets/textures/cameraTexture.png");
		m_SceneEntity = SceneManager::Create(m_Registry);
	}

	Scene::~Scene()
	{
	}

	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
		{
			auto view = src.view<Component>();
			for (auto srcEntity : view)
			{
				entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);
				auto& srcComponent = src.get<Component>(srcEntity);
				dst.emplace_or_replace<Component>(dstEntity, srcComponent);
			}
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
		{
			if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = CreateRef<Scene>();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entitiies in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
			const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
			Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy component (except IDComponent and TagComponent)
		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		m_EntityMap[uuid] = entity;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}

	void Scene::OnRuntimeStart()
	{
		// m_IsRunning = true;

		// OnPhysics2DStart();

		// // scripting
		// {
		// 	ScriptEngine::OnRuntimeStart(this);
		// 	// Instantiate all script entities

		// 	auto view = m_Registry.view<ScriptComponent>();
		// 	for (auto e : view)
		// 	{
		// 		Entity entity = { e, this };
		// 		ScriptEngine::OnCreateEntity(entity);
		// 	}
		// }

	}

	void Scene::OnRuntimeStop()
	{
		m_IsRunning = false;


		// ScriptEngine::OnRuntimeStop();
	}

	void Scene::OnSimulationStart()
	{
	}

	void Scene::OnSimulationStop()
	{
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		// Update scripts
		// {
		
		// 	// C# Entity OnUpdate
		// 	auto view = m_Registry.view<ScriptComponent>();
		// 	for (auto e : view)
		// 	{
		// 		Entity entity = { e, this };
		// 		ScriptEngine::OnUpdateEntity(entity, ts);
		// 	}

		// 	m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
		// 	{
		// 		// TODO: Move to Scene::OnScenePlay
		// 		if (!nsc.Instance)
		// 		{
		// 			nsc.Instance = nsc.InstantiateScript();
		// 			nsc.Instance->m_Entity = Entity{ entity, this };
		// 			nsc.Instance->OnCreate();
		// 		}

		// 		nsc.Instance->OnUpdate(ts);
		// 	});
		// }

		// Physics
		// {
		// 	const int32_t velocityIterations = 6;
		// 	const int32_t positionIterations = 2;

		// 	m_PhysicsWorld->Step(ts, velocityIterations, positionIterations);

		// 	// Retrieve transform from Box2D
		// 	auto view = m_Registry.view<Rigidbody2DComponent>();
		// 	for (auto e : view)
		// 	{
		// 		Entity entity = { e,this };
		// 		auto& transform = entity.GetComponent<TransformComponent>();
		// 		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

		// 		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		// 		const auto& position = body->GetPosition();
		// 		transform.Translation.x = position.x;
		// 		transform.Translation.y = position.y;
		// 		transform.Rotation.z = body->GetAngle();
		// 	}
		// }

		// Render 3D
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);
				
				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transform.GetTransform();
					break;
				}
			}
		}

		if (mainCamera)
		{
			Renderer3D::BeginScene(*mainCamera, cameraTransform);

			{
				auto directionalLightsView = m_Registry.view<TransformComponent, DirectionalLightComponent>();
				for (auto entity : directionalLightsView)
				{
					auto [transform, light] = directionalLightsView.get<TransformComponent, DirectionalLightComponent>(entity);
					ShadowComponent* shadow = m_Registry.try_get<ShadowComponent>(entity);
					Renderer3D::DrawDirectionalLight(transform.GetTransform(), light, shadow, (int)entity);
				}

				auto pointLightsView = m_Registry.view<TransformComponent, PointLightComponent>();
				for (auto entity : pointLightsView)
				{
					auto [transform, light] = pointLightsView.get<TransformComponent, PointLightComponent>(entity);
					ShadowComponent* shadow = m_Registry.try_get<ShadowComponent>(entity);
					Renderer3D::DrawPointLight(transform.GetTransform(), light, shadow, (int)entity);
				}

				auto spotLightsView = m_Registry.view<TransformComponent, SpotLightComponent>();
				for (auto entity : spotLightsView)
				{
					auto [transform, light] = spotLightsView.get<TransformComponent, SpotLightComponent>(entity);
					ShadowComponent* shadow = m_Registry.try_get<ShadowComponent>(entity);
					Renderer3D::DrawSpotLight(transform.GetTransform(), light, shadow, (int)entity);
				}
			}

			// Draw Hdr
			{
				auto view = m_Registry.view<HdrSkyboxComponent>();
				for (auto entity : view)
				{
					auto hdr = view.get<HdrSkyboxComponent>(entity);
					Renderer3D::DrawHdrSkybox(hdr, (int)entity);
				}
			}

			// Draw sprites
			{
				auto view = m_Registry.view<TransformComponent, SphereRendererComponent>();
				for (auto entity : view)
				{
					auto [transform, sprite] = view.get<TransformComponent, SphereRendererComponent>(entity);
					Renderer3D::DrawSphere(transform.GetTransform(), sprite, (int)entity);
				}
			}

			{
				auto view = m_Registry.view<TransformComponent, ModelRendererComponent>();
				for (auto entity : view)
				{
					auto [transform, model] = view.get<TransformComponent, ModelRendererComponent>(entity);
					Renderer3D::DrawModel(transform.GetTransform(), model, (int)entity);
				}
			}

			Renderer3D::EndScene();
		}

	}

	void Scene::OnUpdateSimulation(Timestep ts, EditorCamera& camera)
	{
		// Physics
		// {
		// 	const int32_t velocityIterations = 6;
		// 	const int32_t positionIterations = 2;
		// 	m_PhysicsWorld->Step(ts, velocityIterations, positionIterations);

		// 	// Retrieve transform from Box2D
		// 	auto view = m_Registry.view<Rigidbody2DComponent>();
		// 	for (auto e : view)
		// 	{
		// 		Entity entity = { e,this };
		// 		auto& transform = entity.GetComponent<TransformComponent>();
		// 		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

		// 		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		// 		const auto& position = body->GetPosition();
		// 		transform.Translation.x = position.x;
		// 		transform.Translation.y = position.y;
		// 		transform.Rotation.z = body->GetAngle();
		// 	}
		// }

		RenderScene(camera);
	}

	void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera)
	{
		RenderScene(camera);
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	void Scene::DuplicateEntity(Entity entity)
	{
		Entity newEntity = CreateEntity(entity.GetName());
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		// TODO(Yan) Maybe should be assert
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			return { m_EntityMap.at(uuid), this };

		return {};
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			const auto& camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
				return Entity{entity, this};
		}
		return {};
	}

	void Scene::RenderScene(EditorCamera& camera)
	{
		Renderer3D::BeginScene(camera);

		// Draw camera
		{
			// auto view = m_Registry.view<TransformComponent, CameraComponent>();
			// for (auto entity : view)
			// {
			// 	auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

			// 	Renderer3D::DrawCamera(transform.GetTransform(), m_CameraTexture, (int)entity);
			// }
		}

		{
			auto directionalLightsView = m_Registry.view<TransformComponent, DirectionalLightComponent>();
			for (auto entity : directionalLightsView)
			{
				auto [transform, light] = directionalLightsView.get<TransformComponent, DirectionalLightComponent>(entity);
				ShadowComponent *shadow = m_Registry.try_get<ShadowComponent>(entity);
				Renderer3D::DrawDirectionalLight(transform.GetTransform(), light, shadow, (int)entity);
			}

			auto pointLightsView = m_Registry.view<TransformComponent, PointLightComponent>();
			for (auto entity : pointLightsView)
			{
				auto [transform, light] = pointLightsView.get<TransformComponent, PointLightComponent>(entity);
				ShadowComponent* shadow = m_Registry.try_get<ShadowComponent>(entity);
				Renderer3D::DrawPointLight(transform.GetTransform(), light, shadow, (int)entity);
			}

			auto spotLightsView = m_Registry.view<TransformComponent, SpotLightComponent>();
			for (auto entity : spotLightsView)
			{
				auto [transform, light] = spotLightsView.get<TransformComponent, SpotLightComponent>(entity);
				ShadowComponent *shadow = m_Registry.try_get<ShadowComponent>(entity);
				Renderer3D::DrawSpotLight(transform.GetTransform(), light, shadow, (int)entity);
			}
		}

		// Draw Hdr
		{
			auto view = m_Registry.view<HdrSkyboxComponent>();
			for (auto entity : view)

			{
				auto hdr = view.get<HdrSkyboxComponent>(entity);
				Renderer3D::DrawHdrSkybox(hdr, (int)entity);
			}
		}

		// Draw sprites
		{
			auto view = m_Registry.view<TransformComponent, SphereRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, sprite] = view.get<TransformComponent, SphereRendererComponent>(entity);
				Renderer3D::DrawSphere(transform.GetTransform(), sprite, (int)entity);
			}
		}

		{
			auto view = m_Registry.view<TransformComponent, ModelRendererComponent>();
			for (auto entity : view)
			{
				auto [transform, model] = view.get<TransformComponent, ModelRendererComponent>(entity);
				Renderer3D::DrawModel(transform.GetTransform(), model, (int)entity);
			}
		}

		Renderer3D::EndScene();
	}

	void Scene::NewScene()
	{
		{
			auto uuid = UUID();
			Entity entity = {m_Registry.create(), this};
			entity.AddComponent<IDComponent>();

			auto &hdrSkybox = entity.AddComponent<HdrSkyboxComponent>();
			hdrSkybox.Path = "Assets/textures/hdr/newport_loft.hdr";

			auto &tag = entity.AddComponent<TagComponent>();
			tag.Tag = "Hdr Skybox";

			m_EntityMap[uuid] = entity;
		}

		{
			auto uuid = UUID();
			Entity entity = {m_Registry.create(), this};
			entity.AddComponent<IDComponent>();
			auto &transform = entity.AddComponent<TransformComponent>();
			transform.Translation = glm::vec3(0.0f, 0.0f, 10.0f);
			auto & camera = entity.AddComponent<CameraComponent>();
			camera.Camera.SetProjectionType(SceneCamera::ProjectionType::Perspective);

			auto &tag = entity.AddComponent<TagComponent>();
			tag.Tag = "Camera";

			m_EntityMap[uuid] = entity;
		}

		// int n = 300;
		// for (int i = 0; i < n; i++)
		// {
		// 	auto uuid = UUID();
		// 	Entity entity = {m_Registry.create(), this};
		// 	entity.AddComponent<IDComponent>();
		// 	auto &transform = entity.AddComponent<TransformComponent>();
		// 	transform.Translation = glm::vec3(-4.0f, 2.2f, 0.0f);

		// 	auto &sphere = entity.AddComponent<SphereRendererComponent>();
		// 	sphere.Material->SetTexture(TextureType::Albedo, "Assets/textures/pbr/grass/albedo.png");
		// 	sphere.Material->SetTexture(TextureType::Normal, "Assets/textures/pbr/grass/normal.png");
		// 	sphere.Material->SetTexture(TextureType::Metallic, "Assets/textures/pbr/grass/metallic.png");
		// 	sphere.Material->SetTexture(TextureType::Roughness, "Assets/textures/pbr/grass/roughness.png");
		// 	sphere.Material->SetTexture(TextureType::AmbientOcclusion, "Assets/textures/pbr/grass/ao.png");
		// 	sphere.Material->SetAlbedo(glm::vec4(1.0f));

		// 	auto &tag = entity.AddComponent<TagComponent>();
		// 	tag.Tag = "Grass";

		// 	m_EntityMap[uuid] = entity;
		// }

		// {
		// 	auto uuid = UUID();
		// 	Entity entity = {m_Registry.create(), this};
		// 	entity.AddComponent<IDComponent>();
		// 	auto &transform = entity.AddComponent<TransformComponent>();
		// 	transform.Translation = glm::vec3(-2.0f, 2.2f, 0.0f);

		// 	auto &sphere = entity.AddComponent<SphereRendererComponent>();
		// 	sphere.Material->SetTexture(TextureType::Albedo, "Assets/textures/pbr/gold/albedo.png");
		// 	sphere.Material->SetTexture(TextureType::Normal, "Assets/textures/pbr/gold/normal.png");
		// 	sphere.Material->SetTexture(TextureType::Metallic, "Assets/textures/pbr/gold/metallic.png");
		// 	sphere.Material->SetTexture(TextureType::Roughness, "Assets/textures/pbr/gold/roughness.png");
		// 	sphere.Material->SetTexture(TextureType::AmbientOcclusion, "Assets/textures/pbr/gold/ao.png");
		// 	sphere.Material->SetAlbedo(glm::vec4(1.0f));

		// 	auto &tag = entity.AddComponent<TagComponent>();
		// 	tag.Tag = "Gold";

		// 	m_EntityMap[uuid] = entity;
		// }

		{
			auto uuid = UUID();
			Entity entity = {m_Registry.create(), this};
			entity.AddComponent<IDComponent>();
			auto &transform = entity.AddComponent<TransformComponent>();
			transform.Translation = glm::vec3(-1.0f, 0.5f, 0.0f);

			auto &sphere = entity.AddComponent<SphereRendererComponent>();
			sphere.Material->SetTexture(TextureType::Albedo, "Assets/textures/pbr/plastic/albedo.png");
			sphere.Material->SetTexture(TextureType::Normal, "Assets/textures/pbr/plastic/normal.png");
			sphere.Material->SetTexture(TextureType::Metallic, "Assets/textures/pbr/plastic/metallic.png");
			sphere.Material->SetTexture(TextureType::Roughness, "Assets/textures/pbr/plastic/roughness.png");
			sphere.Material->SetTexture(TextureType::AmbientOcclusion, "Assets/textures/pbr/plastic/ao.png");
			sphere.Material->SetAlbedo(glm::vec4(1.0f));

			auto &tag = entity.AddComponent<TagComponent>();
			tag.Tag = "Plastic";

			m_EntityMap[uuid] = entity;
		}

		{
			auto uuid = UUID();
			Entity entity = {m_Registry.create(), this};
			entity.AddComponent<IDComponent>();
			auto &transform = entity.AddComponent<TransformComponent>();
			transform.Translation = glm::vec3(1.0f, 0.5f, 0.0f);

			auto &sphere = entity.AddComponent<SphereRendererComponent>();
			sphere.Material->SetTexture(TextureType::Albedo, "Assets/textures/pbr/plastic/albedo.png");
			sphere.Material->SetTexture(TextureType::Normal, "Assets/textures/pbr/plastic/normal.png");
			sphere.Material->SetTexture(TextureType::Metallic, "Assets/textures/pbr/plastic/metallic.png");
			sphere.Material->SetTexture(TextureType::Roughness, "Assets/textures/pbr/plastic/roughness.png");
			sphere.Material->SetTexture(TextureType::AmbientOcclusion, "Assets/textures/pbr/plastic/ao.png");
			sphere.Material->SetAlbedo(glm::vec4(1.0f));

			auto &tag = entity.AddComponent<TagComponent>();
			tag.Tag = "Plastic";

			m_EntityMap[uuid] = entity;
		}

		{
			auto uuid = UUID();
			Entity entity = {m_Registry.create(), this};
			entity.AddComponent<IDComponent>();
			auto &transform = entity.AddComponent<TransformComponent>();
			transform.Translation = glm::vec3(-6.0f, -4.0f, 0.0f);
			transform.Scale = glm::vec3(5.0f, 5.0f, 5.0f);

			auto &sphere = entity.AddComponent<SphereRendererComponent>();
			sphere.Material->SetTexture(TextureType::Albedo, "Assets/textures/pbr/plastic/albedo.png");
			sphere.Material->SetTexture(TextureType::Normal, "Assets/textures/pbr/plastic/normal.png");
			sphere.Material->SetTexture(TextureType::Metallic, "Assets/textures/pbr/plastic/metallic.png");
			sphere.Material->SetTexture(TextureType::Roughness, "Assets/textures/pbr/plastic/roughness.png");
			sphere.Material->SetTexture(TextureType::AmbientOcclusion, "Assets/textures/pbr/plastic/ao.png");
			sphere.Material->SetAlbedo(glm::vec4(1.0f));

			auto &tag = entity.AddComponent<TagComponent>();
			tag.Tag = "chassis";

			m_EntityMap[uuid] = entity;
		}

		// {
		// 	auto uuid = UUID();
		// 	Entity entity = {m_Registry.create(), this};
		// 	entity.AddComponent<IDComponent>();
		// 	auto &transform = entity.AddComponent<TransformComponent>();
		// 	transform.Translation = glm::vec3(2.0f, 2.2f, 0.0f);

		// 	auto &sphere = entity.AddComponent<SphereRendererComponent>();
		// 	sphere.Material->SetTexture(TextureType::Albedo, "Assets/textures/pbr/rusted_iron/albedo.png");
		// 	sphere.Material->SetTexture(TextureType::Normal, "Assets/textures/pbr/rusted_iron/normal.png");
		// 	sphere.Material->SetTexture(TextureType::Metallic, "Assets/textures/pbr/rusted_iron/metallic.png");
		// 	sphere.Material->SetTexture(TextureType::Roughness, "Assets/textures/pbr/rusted_iron/roughness.png");
		// 	sphere.Material->SetTexture(TextureType::AmbientOcclusion, "Assets/textures/pbr/rusted_iron/ao.png");
		// 	sphere.Material->SetAlbedo(glm::vec4(1.0f));

		// 	auto &tag = entity.AddComponent<TagComponent>();
		// 	tag.Tag = "Rusted_iron";

		// 	m_EntityMap[uuid] = entity;
		// }

		// {
		// 	auto uuid = UUID();
		// 	Entity entity = {m_Registry.create(), this};
		// 	entity.AddComponent<IDComponent>();
		// 	auto &transform = entity.AddComponent<TransformComponent>();
		// 	transform.Translation = glm::vec3(4.0f, 2.2f, 0.0f);

		// 	auto &sphere = entity.AddComponent<SphereRendererComponent>();
		// 	sphere.Material->SetTexture(TextureType::Albedo, "Assets/textures/pbr/wall/albedo.png");
		// 	sphere.Material->SetTexture(TextureType::Normal, "Assets/textures/pbr/wall/normal.png");
		// 	sphere.Material->SetTexture(TextureType::Metallic, "Assets/textures/pbr/wall/metallic.png");
		// 	sphere.Material->SetTexture(TextureType::Roughness, "Assets/textures/pbr/wall/roughness.png");
		// 	sphere.Material->SetTexture(TextureType::AmbientOcclusion, "Assets/textures/pbr/wall/ao.png");
		// 	sphere.Material->SetAlbedo(glm::vec4(1.0f));

		// 	auto &tag = entity.AddComponent<TagComponent>();
		// 	tag.Tag = "Wall";

		// 	m_EntityMap[uuid] = entity;
		// }

		// {
		// 	auto uuid = UUID();
		// 	Entity entity = {m_Registry.create(), this};
		// 	entity.AddComponent<IDComponent>();
		// 	entity.AddComponent<TransformComponent>();

		// 	auto &obj = entity.AddComponent<ModelRendererComponent>();
		// 	obj.FlipUV = true;
		// 	obj.ModelPath = "Assets/objs/Spaceship/Spaceship.obj";
		// 	obj.Model = ModelManager::Get().GetModel(obj.ModelPath);

		// 	auto &tag = entity.AddComponent<TagComponent>();
		// 	tag.Tag = "obj Model 0";

		// 	m_EntityMap[uuid] = entity;
		// }

		// {
		// 	auto uuid = UUID();
		// 	Entity entity = {m_Registry.create(), this};
		// 	entity.AddComponent<IDComponent>();
		// 	auto &transform = entity.AddComponent<TransformComponent>();
		// 	transform.Translation = glm::vec3(0.0f, 0.0f, 0.0f);

		// 	auto &obj = entity.AddComponent<ModelRendererComponent>();
		// 	obj.FlipUV = true;
		// 	obj.ModelPath = "Assets/objs/backpack/backpack.obj";
		// 	obj.Model = ModelManager::Get().GetModel(obj.ModelPath);
		// 	// obj.Model = Model::Create(obj.ModelPath, obj.FlipUV);

		// 	auto &tag = entity.AddComponent<TagComponent>();
		// 	tag.Tag = "obj Model 1";

		// 	m_EntityMap[uuid] = entity;
		// }

		{
			auto uuid = UUID();
			Entity entity = {m_Registry.create(), this};
			entity.AddComponent<IDComponent>();
			auto &transform = entity.AddComponent<TransformComponent>();
			transform.Rotation = glm::vec3(-0.5f, 1.0f, 0.0f);

			auto &light = entity.AddComponent<DirectionalLightComponent>();

			auto &tag = entity.AddComponent<TagComponent>();
			tag.Tag = "Directional Light";
			m_EntityMap[uuid] = entity;
		}

		// {
		// 	auto uuid = UUID();
		// 	Entity entity = {m_Registry.create(), this};
		// 	entity.AddComponent<IDComponent>();
		// 	auto &transform = entity.AddComponent<TransformComponent>();
		// 	transform.Translation = glm::vec3(1.0f, 6.0f, 0.0f);

		// 	entity.AddComponent<PointLightComponent>();

		// 	auto &tag = entity.AddComponent<TagComponent>();
		// 	tag.Tag = "Point Light1";

		// 	m_EntityMap[uuid] = entity;
		// }

		// {
		// 	auto uuid = UUID();
		// 	Entity entity = {m_Registry.create(), this};
		// 	entity.AddComponent<IDComponent>();
		// 	auto &transform = entity.AddComponent<TransformComponent>();
		// 	transform.Translation = glm::vec3(-1.0f, 6.0f, 0.0f);

		// 	entity.AddComponent<PointLightComponent>();

		// 	auto &tag = entity.AddComponent<TagComponent>();
		// 	tag.Tag = "Point Light2";

		// 	m_EntityMap[uuid] = entity;
		// }

		{
			auto uuid = UUID();
			Entity entity = {m_Registry.create(), this};
			entity.AddComponent<IDComponent>();
			auto &transform = entity.AddComponent<TransformComponent>();
			transform.Translation = glm::vec3(6.0f, 4.0f, 4.0f);
			transform.Rotation = glm::vec3(-0.5f, 1.0f, 0.0f);
			

			auto& light = entity.AddComponent<SpotLightComponent>();
			light.Intensity = 10.0f;
			light.Radius = 20.0f;

			auto &tag = entity.AddComponent<TagComponent>();
			tag.Tag = "Spot Light";
			m_EntityMap[uuid] = entity;
		}
	}

	template <typename T>
	void Scene::OnComponentAdded(Entity entity, T &component)
	{
	}


	template <>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<DirectionalLightComponent>(Entity entity, DirectionalLightComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<PointLightComponent>(Entity entity, PointLightComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<SpotLightComponent>(Entity entity, SpotLightComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<ShadowComponent>(Entity entity, ShadowComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<SphereRendererComponent>(Entity entity, SphereRendererComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<ModelRendererComponent>(Entity entity, ModelRendererComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<HdrSkyboxComponent>(Entity entity, HdrSkyboxComponent &component)
	{
	}

	template <>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent &component)
	{
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}
}
