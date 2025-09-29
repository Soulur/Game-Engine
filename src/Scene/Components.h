#pragma once

#include "src/Scene/SceneCamera.h"
#include "src/Core/UUID.h"
#include "src/Renderer/Texture.h"
#include "src/Renderer/Sphere.h"
#include "src/Renderer/Model.h"

#include "src/Renderer/Manager/MaterialManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Mc 
{

	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct DirectionalLightComponent
	{
		glm::vec3 Color{1.0f, 1.0f, 1.0f};
		float Intensity = 1.0f;
		bool CastsShadows = false;

		DirectionalLightComponent() = default;
		DirectionalLightComponent(const DirectionalLightComponent &) = default;
	};

	struct PointLightComponent
	{
		glm::vec3 Color{1.0f, 1.0f, 1.0f};
		float Intensity = 1.0f;
		bool CastsShadows = false;

		float Radius = 10.0f;

		PointLightComponent() = default;
		PointLightComponent(const PointLightComponent &) = default;
	};

	struct SpotLightComponent
	{
		glm::vec3 Color{1.0f, 1.0f, 1.0f};
		float Intensity = 1.0f;
		bool CastsShadows = false;

		float Radius = 10.0f;
		float InnerConeAngle = glm::radians(10.0f);
		float OuterConeAngle = glm::radians(20.0f);

		SpotLightComponent() = default;
		SpotLightComponent(const SpotLightComponent &) = default;
	};

	struct ShadowComponent
	{
		unsigned int Resolution = 1024;
		float NearPlane = 1.0f;
		float FarPlane = 25.0f;
	};

	struct SphereRendererComponent
	{
		Ref<Material> Material = MaterialManager::Get().AddMaterial();

		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float TilingFactor = 1.0f;

		bool ReceivesPBR = false;
		bool ReceivesIBL = false;
		bool ReceivesLight = false;
		bool ReceivesShadow = false;
		bool ProjectionShadow = false;

		SphereRendererComponent() = default;
		SphereRendererComponent(const SphereRendererComponent &) = default;
		SphereRendererComponent(const glm::vec4& color)
			: Color(color) {}
	};

	struct ModelRendererComponent
	{
		std::string ModelPath;
		Ref<Model> Model;
		glm::vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
		
		bool FlipUV = false;

		bool ReceivesPBR = false;
		bool ReceivesIBL = false;
		bool ReceivesLight = false;

		bool GammaCorrection = false;

		Ref<Mesh> CurrentMesh = nullptr; // 当前批次正在渲染的模型

		ModelRendererComponent() = default;
		ModelRendererComponent(const ModelRendererComponent &) = default;
	};

	struct HdrSkyboxComponent
	{
		std::string Path;

		HdrSkyboxComponent() = default;
		HdrSkyboxComponent(const HdrSkyboxComponent &) = default;
	};

	// Forward declaration

	// Physics

	template<typename... Component>
	struct ComponentGroup
	{
	};

	using AllComponents =
		ComponentGroup<TransformComponent, CameraComponent,
		DirectionalLightComponent,PointLightComponent,SpotLightComponent,
		ShadowComponent,
		SphereRendererComponent,
		ModelRendererComponent, HdrSkyboxComponent
		>;
}
