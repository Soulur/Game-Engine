#pragma once

#include "src/Core/Base.h"
#include "src/Renderer/VertexArray.h"
#include "src/Renderer/Shader.h"
#include "src/Renderer/Texture.h"

#include "src/Renderer/Camera.h"
#include "src/Renderer/OrthographicCamera.h"
#include "src/Renderer/EditorCamera.h"

#include "src/Scene/Components.h"

#include <entt/entt.hpp>

namespace Mc
{
	class Renderer3D
	{
	public:
		static void InitSphereGeometry();

		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera &camera, const glm::mat4 &transform);
		static void BeginScene(const EditorCamera &camera);
		static void BeginScene(const OrthographicCamera &camera);
		static void EndScene();

		static void StartBatch();
		static void FlushLights();
		static void FlushPointShadows();
		static void FlushDirectionalShadows();
		static void Flush();
		static void NextBatch();
		
	public:
		// Camera
		// static void DrawCramera(const glm::mat4 &transform, int entityID = -1);

		// Sphere
		static void DrawSphere(const glm::mat4 &transform, SphereRendererComponent &src, int entityID);

		static void DrawModel(const glm::mat4 &transform, ModelRendererComponent &src, int entityID);

		static void DrawDirectionalLight(const glm::mat4 &transform, DirectionalLightComponent &src, ShadowComponent *shadow, int entityID);
		static void DrawPointLight(const glm::mat4 &transform, PointLightComponent &src, ShadowComponent *shadow, int entityID);
		static void DrawSpotLight(const glm::mat4 &transform, SpotLightComponent &src, int entityID);

		static void AddDirectionalShadow(entt::entity entity, unsigned int resolution);
		static void DelDirectionalShadow(entt::entity entity);

		static void AddPointShadow(entt::entity entity, unsigned int resolution);
		static void DelPointShadow(entt::entity entity);
		static void AddSpotShadow();

		static void DrawHdrSkybox(HdrSkyboxComponent &src, int entityID);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;

			uint32_t SphereCount = 0;

			uint32_t ModelCount = 0;
			uint32_t MeshCount = 0;
		};
		static void ResetStats();
		static Statistics GetStats();
	};
}
