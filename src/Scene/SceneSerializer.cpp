#include "SceneSerializer.h"

#include "src/Scene/Entity.h"
#include "src/Scene/Components.h"
#include "src/Renderer/Manager/ModelManager.h"

#include <fstream>

#include <yaml-cpp/yaml.h>

namespace YAML 
{

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

}

namespace Mc 
{

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID(); // TODO: Entity ID goes here

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<HdrSkyboxComponent>())
		{
			out << YAML::Key << "HdrSkyboxComponent";
			out << YAML::BeginMap; // HdrSkyboxComponent

			auto &hdrSkyboxComponent = entity.GetComponent<HdrSkyboxComponent>();
			if (!hdrSkyboxComponent.Path.empty())
				out << YAML::Key << "HdrPath" << YAML::Value << hdrSkyboxComponent.Path;

				out << YAML::EndMap; // HdrSkyboxComponent
		}

		if (entity.HasComponent<DirectionalLightComponent>())
		{
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap; // DirectionalLightComponent

			auto &directionalLightComponent = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "Intensity" << YAML::Value << directionalLightComponent.Intensity;
			out << YAML::Key << "Color" << YAML::Value << directionalLightComponent.Color;
			out << YAML::Key << "CastsShadows" << YAML::Value << directionalLightComponent.CastsShadows;

			out << YAML::EndMap; // DirectionalLightComponent
		}

		if (entity.HasComponent<PointLightComponent>())
		{
			out << YAML::Key << "PointLightComponent";
			out << YAML::BeginMap; // PointLightComponent

			auto &pointLightComponent = entity.GetComponent<PointLightComponent>();
			out << YAML::Key << "Intensity" << YAML::Value << pointLightComponent.Intensity;
			out << YAML::Key << "Color" << YAML::Value << pointLightComponent.Color;
			out << YAML::Key << "CastsShadows" << YAML::Value << pointLightComponent.CastsShadows;
			out << YAML::Key << "Radius" << YAML::Value << pointLightComponent.Radius;

			out << YAML::EndMap; // PointLightComponent
		}

		if (entity.HasComponent<SpotLightComponent>())
		{
			out << YAML::Key << "SpotLightComponent";
			out << YAML::BeginMap; // SpotLightComponent

			auto &spotLightComponent = entity.GetComponent<SpotLightComponent>();
			out << YAML::Key << "Intensity" << YAML::Value << spotLightComponent.Intensity;
			out << YAML::Key << "Color" << YAML::Value << spotLightComponent.Color;
			out << YAML::Key << "CastsShadows" << YAML::Value << spotLightComponent.CastsShadows;
			out << YAML::Key << "Radius" << YAML::Value << spotLightComponent.Radius;
			out << YAML::Key << "InnerConeAngle" << YAML::Value << spotLightComponent.InnerConeAngle;
			out << YAML::Key << "OuterConeAngle" << YAML::Value << spotLightComponent.OuterConeAngle;

			out << YAML::EndMap; // SpotLightComponent
		}

		if (entity.HasComponent<SphereRendererComponent>())
		{
			out << YAML::Key << "SphereRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto &sphereRendererComponent = entity.GetComponent<SphereRendererComponent>();
			{
				out << YAML::Key << "AlbedoValue" << YAML::Value << sphereRendererComponent.Material->GetAlbedo();
				out << YAML::Key << "AOValue" << YAML::Value << sphereRendererComponent.Material->GetAO();
				out << YAML::Key << "EmissiveValue" << YAML::Value << sphereRendererComponent.Material->GetEmissive();
				out << YAML::Key << "MetallicValue" << YAML::Value << sphereRendererComponent.Material->GetMetallic();
				out << YAML::Key << "RoughnessValue" << YAML::Value << sphereRendererComponent.Material->GetRoughness();

				std::vector<std::string> arr = {"AlbedoMap", "NormalMap", "MetallicMap", "RoughnessMap", "AmbientOcclusionMap", "EmissiveMap", "HeightMap"};
				for (int i = 0; i < (int)TextureType::Count; i++)
				{
					if (sphereRendererComponent.Material->GetTexture((TextureType)i))
					{
						out << YAML::Key << arr[i] << YAML::Value << sphereRendererComponent.Material->GetTexture((TextureType)i)->GetPath();
					}
				}
			}
			out << YAML::Key << "Color" << YAML::Value << sphereRendererComponent.Color;			
			out << YAML::Key << "TilingFactor" << YAML::Value << sphereRendererComponent.TilingFactor;
			out << YAML::Key << "ReceivesPBR" << YAML::Value << sphereRendererComponent.ReceivesPBR;
			out << YAML::Key << "ReceivesIBL" << YAML::Value << sphereRendererComponent.ReceivesIBL;
			out << YAML::Key << "ReceivesLight" << YAML::Value << sphereRendererComponent.ReceivesLight;

			out << YAML::EndMap; // SpriteRendererComponent
		}

		if (entity.HasComponent<ModelRendererComponent>())
		{
			out << YAML::Key << "ModelRendererComponent";
			out << YAML::BeginMap; // ModelRendererComponent

			auto &modelRendererComponent = entity.GetComponent<ModelRendererComponent>();
			if (modelRendererComponent.Model)
			{
				out << YAML::Key << "ModelPath" << YAML::Value << modelRendererComponent.ModelPath;
				for (auto &mesh : modelRendererComponent.Model->GetMeshs())
					if (!mesh->GetMaterial()->GetName().empty())
						out << YAML::Key << mesh->GetName() << YAML::Value << mesh->GetMaterial()->GetName();
			}
			out << YAML::Key << "Color" << YAML::Value << modelRendererComponent.Color;
			out << YAML::Key << "FlipUV" << YAML::Value << modelRendererComponent.FlipUV;
			out << YAML::Key << "ReceivesPBR" << YAML::Value << modelRendererComponent.ReceivesPBR;
			out << YAML::Key << "ReceivesIBL" << YAML::Value << modelRendererComponent.ReceivesIBL;
			out << YAML::Key << "ReceivesLight" << YAML::Value << modelRendererComponent.ReceivesLight;
			out << YAML::Key << "GammaCorrection" << YAML::Value << modelRendererComponent.GammaCorrection;

			out << YAML::EndMap; // ModelRendererComponent
		}

		out << YAML::EndMap; // Entity
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		auto view = m_Scene->m_Registry.view<entt::entity>();
		for (auto entityID : view)
		{
			Entity entity = {entityID, m_Scene.get()};
			if (!entity)
				return;

			SerializeEntity(out, entity);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		// Not implemented
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			return false;
		}

		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>(); // TODO

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();


				Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					tc.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();

					auto cameraProps = cameraComponent["Camera"];
					cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());

					cc.Camera.SetPerspectiveVerticalFOV(cameraProps["PerspectiveFOV"].as<float>());
					cc.Camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
					cc.Camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());

					cc.Camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
					cc.Camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
					cc.Camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

					cc.Primary = cameraComponent["Primary"].as<bool>();
					cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
				}

				auto hdrSkyboxComponent = entity["HdrSkyboxComponent"];
				if (hdrSkyboxComponent)
				{
					auto &src = deserializedEntity.AddComponent<HdrSkyboxComponent>();
					if (hdrSkyboxComponent["HdrPath"])
						src.Path = hdrSkyboxComponent["HdrPath"].as<std::string>();
				}

				auto directionalLightComponent = entity["DirectionalLightComponent"];
				if (directionalLightComponent)
				{
					auto& src = deserializedEntity.AddComponent<DirectionalLightComponent>();
					src.Intensity = directionalLightComponent["Intensity"].as<float>();
					src.Color = directionalLightComponent["Color"].as<glm::vec3>();
					src.CastsShadows = directionalLightComponent["CastsShadows"].as<bool>();
				}

				auto pointLightComponent = entity["PointLightComponent"];
				if (pointLightComponent)
				{
					auto &src = deserializedEntity.AddComponent<PointLightComponent>();
					src.Intensity = pointLightComponent["Intensity"].as<float>();
					src.Color = pointLightComponent["Color"].as<glm::vec3>();
					src.CastsShadows = pointLightComponent["CastsShadows"].as<bool>();
					src.Radius = pointLightComponent["Radius"].as<float>();
				}

				auto spotLightComponent = entity["SpotLightComponent"];
				if (spotLightComponent)
				{
					auto &src = deserializedEntity.AddComponent<SpotLightComponent>();
					src.Intensity = pointLightComponent["Intensity"].as<float>();
					src.Color = pointLightComponent["Color"].as<glm::vec3>();
					src.CastsShadows = pointLightComponent["CastsShadows"].as<bool>();
					src.Radius = pointLightComponent["Radius"].as<float>();
					src.InnerConeAngle = pointLightComponent["InnerConeAngle"].as<float>();
					src.OuterConeAngle = pointLightComponent["OuterConeAngle"].as<float>();
				}

				auto sphereRendererComponent = entity["SphereRendererComponent"];
				if (sphereRendererComponent)
				{
					auto &src = deserializedEntity.AddComponent<SphereRendererComponent>();
					{
						src.Material->SetAlbedo(sphereRendererComponent[ "AlbedoValue"].as<glm::vec4>());
						src.Material->SetAO(sphereRendererComponent[ "AOValue"].as<float>());
						src.Material->SetEmissive(sphereRendererComponent[ "EmissiveValue"].as<glm::vec3>());
						src.Material->SetMetallic(sphereRendererComponent[ "MetallicValue"].as<float>());
						src.Material->SetRoughness(sphereRendererComponent[ "RoughnessValue"].as<float>());

						std::vector<std::string> arr = {"AlbedoMap", "NormalMap", "MetallicMap", "RoughnessMap", "AmbientOcclusionMap", "EmissiveMap", "HeightMap"};
						for (int i = 0; i < (int)TextureType::Count; i++)
						{
							if (sphereRendererComponent[arr[i]])
							{
								src.Material->SetTexture((TextureType)i, sphereRendererComponent[arr[i]].as<std::string>());
							}
						}
					}

					src.Color = sphereRendererComponent["Color"].as<glm::vec4>();
					if (sphereRendererComponent["TilingFactor"])
						src.TilingFactor = sphereRendererComponent["TilingFactor"].as<float>();
					src.ReceivesPBR = sphereRendererComponent["ReceivesPBR"].as<bool>();
					src.ReceivesIBL = sphereRendererComponent["ReceivesIBL"].as<bool>();
					src.ReceivesLight = sphereRendererComponent["ReceivesLight"].as<bool>();
				}

				auto modelRendererComponent = entity["ModelRendererComponent"];
				if (modelRendererComponent)
				{
					auto &src = deserializedEntity.AddComponent<ModelRendererComponent>();
					if (modelRendererComponent["ModelPath"])
					{
						std::string a = modelRendererComponent["ModelPath"].as<std::string>();
						src.ModelPath = a;
						src.Model = ModelManager::Get().GetModel(a);

						for (auto &mesh : src.Model->GetMeshs())
							if (modelRendererComponent[mesh->GetName()])
							{
								std::string materialName = modelRendererComponent[mesh->GetName()].as<std::string>();
								for (auto &material : src.Model->GetMaterials())
									if (material->GetName() == materialName)
										mesh->SetMaterial(material);
							}
					}
					src.Color = modelRendererComponent["Color"].as<glm::vec4>();
					src.FlipUV = modelRendererComponent["FlipUV"].as<bool>();
					src.ReceivesPBR = modelRendererComponent["ReceivesPBR"].as<bool>();
					src.ReceivesIBL = modelRendererComponent["ReceivesIBL"].as<bool>();
					src.ReceivesLight = modelRendererComponent["ReceivesLight"].as<bool>();
					src.GammaCorrection = modelRendererComponent["GammaCorrection"].as<bool>();
				}
			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		// Not implemented
		return false;
	}

}
