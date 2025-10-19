#include "Renderer3D.h"

#include "src/Renderer/Shader.h"
#include "src/Renderer/Texture.h"
#include "src/Renderer/Renderer.h"

#include "src/Renderer/Sphere.h"

#include "src/Renderer/UniformBuffer.h"
#include "src/Renderer/HDRSkybox.h"
#include "src/Renderer/Manager/TextureManager.h"
#include "src/Renderer/Manager/MeshManager.h"
#include "src/Renderer/Manager/HdrManager.h"

#include "src/Renderer/Shadow.h"
#include <entt/entt.hpp>

#include <glad/glad.h>
#include <iostream>

#include <glm/gtx/matrix_decompose.hpp>

namespace Mc
{
	// 简单的错误检查宏
	// #define CHECK_GL_ERROR()                                                                                  \
// 	do                                                                                                    \
// 	{                                                                                                     \
// 		GLenum error = glGetError();                                                                      \
// 		if (error != GL_NO_ERROR)                                                                         \
// 		{                                                                                                 \
// 			std::cerr << "OpenGL Error: " << error << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
// 		}                                                                                                 \
// 	} while (0)

	// SSBO
	// ----------------------------------------------------------------------------------------------------
	struct SphereInstanceData
	{
		glm::mat4 Transform;
		glm::vec4 TintColor;

		// PBR
		glm::vec4 AlbedoColor;
		glm::vec4 EmissiveColor;
		float Roughness;
		float Metallic;
		float AO;

		// 纹理贴图 (对应 Material.h 中的 TextureType 枚举)
		int AlbedoMapIndex;	   // 0
		int NormalMapIndex;	   // 1
		int MetallicMapIndex;  // 2
		int RoughnessMapIndex; // 3
		int AoMapIndex;		   // 4
		int EmissiveMapIndex;  // 5
		int HeightMapIndex;	   // 6

		float TilingFactor;
		int EntityID;

		int ReceivesPBR;
		int ReceivesIBL;
		int ReceivesLight;
		int ReceivesShadow;
	};

	// 静态断言来验证 SphereInstanceData 的大小，确保符合 std140 布局
	// 128
	static_assert(sizeof(SphereInstanceData) % 16 == 0, "SphereInstanceData size mismatch for std140 layout!");

	struct MeshInstanceData
	{
		glm::mat4 Transform;
		glm::vec4 TintColor;

		// PBR
		glm::vec4 AlbedoColor;
		glm::vec4 EmissiveColor;
		float Roughness;
		float Metallic;
		float AO;

		// 纹理贴图 (对应 Material.h 中的 TextureType 枚举)
		int AlbedoMapIndex;	   // 0
		int NormalMapIndex;	   // 1
		int MetallicMapIndex;  // 2
		int RoughnessMapIndex; // 3
		int AoMapIndex;		   // 4
		int EmissiveMapIndex;  // 5
		int HeightMapIndex;	   // 6

		int EntityID;
		int FlipUV;

		int ReceivesPBR;
		int ReceivesIBL;
		int ReceivesLight;
		int ReceivesShadow;
	};

	static_assert(sizeof(MeshInstanceData) % 16 == 0, "MeshInstanceData size mismatch for std140 layout!");

	static const uint32_t MAXLIGHTS = 8;
	// Light
	struct StoredDirectionalLight
	{
		glm::vec3 Direction;
		float Intensity;
		glm::vec3 Color;
		int CastsShadows;

		glm::mat4 LightSpaceMatrix;
		int ShadowMapIndex;
		float padding_0;
		float padding_1;
		float padding_2;
	};

	struct StoredPointLight
	{
		glm::vec3 Position;
		float Intensity;
		glm::vec3 Color;
		float Radius;

		int CastsShadows;
		float FarPlane;
		int ShadowMapIndex;
		float padding_0;
	};

	struct StoredSpotLight
	{
		glm::vec3 Position;
		float Intensity;
		glm::vec3 Color;
		float Radius;
		glm::vec3 Direction;
		float InnerConeCos; // 内锥角的余弦值

		glm::mat4 LightSpaceMatrix;
		float OuterConeCos; // 外锥角的余弦值
		int ShadowMapIndex;
		int CastsShadows;
		float padding_0;
	};

	struct LightData
	{
		// 相机位置
		glm::vec3 CameraPosition;
		float padding1; // 填充

		// 光源数量
		int NumDirectionalLights;
		int NumPointLights;
		int NumSpotLights;
		int padding2;

		StoredDirectionalLight DirectionalLights[MAXLIGHTS];
		StoredPointLight PointLights[MAXLIGHTS];
		StoredSpotLight SpotLights[MAXLIGHTS];
	};

	static_assert(sizeof(LightData) % 16 == 0, "LightData size mismatch for std140 layout!");
	// -------------------------------------------------------------------------------

	struct Renderer3DData
	{
		// Texture
		static const uint32_t MaxTextureSlots = 32;
		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;

		Ref<Texture2D> WhiteTexture;

		// hdr
		// -------------------------------------------------------------------------------
		Ref<HDRSkybox> HdrSkybox;

		// -------------------------------------------------------------------------------
		// Sphere
		// -------------------------------------------------------------------------------
		static const uint32_t MaxSphereCount = 20000;
		uint32_t SphereCount = 0;

		Ref<Sphere> DefaultSphereMesh;
		Ref<Shader> SphereShader;

		// ==== SSBO ====
		Ref<ShaderStorageBuffer> SphereInstanceSSBO;
		std::vector<SphereInstanceData> SphereInstances;

		// Model
		// -------------------------------------------------------------------------------
		uint32_t ModelCount = 0;
		static const uint32_t MaxUniqueModels = 32;		   // 最多支持多少种不同的模型文件
		static const uint32_t MaxInstancesPerModel = 2000; // 每种模型最多支持多少个实例

		struct ModelCallBatch
		{
			std::vector<MeshInstanceData> Instances; // MeshInstanceData 包含 Transform, TintColor 等
			Ref<ShaderStorageBuffer> InstanceSSBO;	 // 为该批次准备的 SSBO
			uint32_t InstanceCount = 0;
		};

		struct ModelCallKey
		{
			uint32_t MeshID;

			bool operator==(const ModelCallKey &other) const
			{
				return MeshID == other.MeshID;
			}
		};

		struct ModelCallKeyHash
		{
			size_t operator()(const ModelCallKey &key) const
			{
				// 简单的哈希组合，可以更复杂
				return std::hash<uint32_t>()(key.MeshID) << 1;
			}
		};

		// 使用哈希表来组织所有批次
		std::unordered_map<ModelCallKey, ModelCallBatch, ModelCallKeyHash> ModelDrawCallBatches;

		Ref<Shader> ModelShader;

		// Light
		// -------------------------------------------------------------------------------
		static const uint32_t MAX_DIRECTIONAL_LIGHTS = MAXLIGHTS; // 着色器中定义的数组大小
		static const uint32_t MAX_POINT_LIGHTS = MAXLIGHTS;
		static const uint32_t MAX_SPOT_LIGHTS = MAXLIGHTS;

		// ssbo
		Ref<ShaderStorageBuffer> LightInstanceSSBO;

		// 用于收集数据的C++容器
		std::vector<StoredDirectionalLight> DirectionalLights;
		std::vector<StoredPointLight> PointLights;
		std::vector<StoredSpotLight> SpotLights;

		// shadow
		// -------------------------------------------------------------------------------
		struct ProjectionShadowData
		{
			glm::mat4 Transform;
			uint32_t VertexArrayID = 0;
			uint32_t IndexCount = 0;
		};
		std::vector<ProjectionShadowData> ProjectionShadowDatas;

		// Point Shadow
		std::unordered_map<int, Ref<PointShadowMap>> PointShadowMaps;
		// shader
		Ref<Shader> PointShadowShader;

		struct StoredPointShadowGPU
		{
			glm::mat4 ShadowTransforms[6];
			glm::vec4 LightPositionFarPlane;
		};

		Ref<ShaderStorageBuffer> PointShadowInstanceSSBO;

		struct StoredPointShadowCPU
		{
			StoredPointShadowGPU GPUData;

			Ref<PointShadowMap> Shadow;
			int Slot;
		};

		std::vector<StoredPointShadowCPU> PointShadows;
		uint32_t PointShadowTextureSlotIndex = 0;
		uint32_t PointShadowTextureStart = 0;

		// Directional Shadow
		std::unordered_map<int, Ref<DirectionalShadowMap>> DirectionalShadowMaps;
		// shader
		Ref<Shader> DirectionalShadowShader;

		struct StoredDirectionalShadowCPU
		{
			glm::mat4 LightSpaceMatrix;

			Ref<DirectionalShadowMap> Shadow;
			int Slot;
		};

		std::vector<StoredDirectionalShadowCPU> DirectionalShadows;
		uint32_t DirectionalShadowTextureSlotIndex = 0;
		uint32_t DirectionalShadowTextureStart = 0;

		// Spot Shadow
		std::unordered_map<int, Ref<SpotShadowMap>> SpotShadowMaps;
		// shader
		Ref<Shader> SpotShadowShader;

		struct StoredSpotShadowCPU
		{
			glm::mat4 LightSpaceMatrix;

			Ref<SpotShadowMap> Shadow;
			int Slot;
		};

		std::vector<StoredSpotShadowCPU> SpotShadows;
		uint32_t SpotShadowTextureSlotIndex = 0;
		uint32_t SpotShadowTextureStart = 0;

		// Camera
		// -------------------------------------------------------------------------------
		struct CameraData
		{
			glm::mat4 Projection;
			glm::mat4 View;
			glm::mat4 ViewProjection;
			glm::vec3 CameraPosition;
		};
		CameraData CameraBuffer;
		// Ref<UniformBuffer> CameraUniformBuffer;

		Renderer3D::Statistics Stats;
	};

	static Renderer3DData s_Data;

	namespace Utils
	{
		std::vector<int> GetMaterialTextureIndices(const Ref<Material> &material)
		{
			std::vector<int> indices((int)TextureType::Count, 0);

			if (!material)
				return indices;

			auto textures = material->Get();

			for (int textureTypeIndex = 0; textureTypeIndex < (int)TextureType::Count; ++textureTypeIndex)
			{
				TextureType type = (TextureType)textureTypeIndex;
				const Ref<Texture2D> &texture = textures.at(type);

				if (texture && texture != s_Data.WhiteTexture)
				{
					int foundSlotIndex = -1;

					for (uint32_t slotIndex = 1; slotIndex < s_Data.TextureSlotIndex; ++slotIndex)
					{
						if (*s_Data.TextureSlots[slotIndex] == *texture)
						{
							foundSlotIndex = slotIndex;
							break;
						}
					}

					if (foundSlotIndex != -1)
					{
						indices[textureTypeIndex] = foundSlotIndex;
					}
					else
					{
						if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
							Renderer3D::NextBatch();

						indices[textureTypeIndex] = s_Data.TextureSlotIndex;
						s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
						s_Data.TextureSlotIndex++;
					}
				}
			}
			return indices;
		}

		std::vector<int> GetMaterialTextureIndices(MaterialComponent &material)
		{
			int TextureSize = 7;

			std::vector<int> indices(TextureSize, 0);

			std::string *mapPointers[] = {
				&material.AlbedoMap,
				&material.NormalMap,
				&material.MetallicMap,
				&material.RoughnessMap,
				&material.AmbientOcclusionMap,
				&material.EmissiveMap,
				&material.HeightMap
			};

			for (int textureTypeIndex = 0; textureTypeIndex < TextureSize; ++textureTypeIndex)
			{
				std::string *currentMapPath = mapPointers[textureTypeIndex];
				if (currentMapPath->empty())
					continue;

				const Ref<Texture2D> &texture = TextureManager::Get().GetTexture(*currentMapPath);

				if (texture && texture != s_Data.WhiteTexture)
				{
					int foundSlotIndex = -1;

					for (uint32_t slotIndex = 1; slotIndex < s_Data.TextureSlotIndex; ++slotIndex)
					{
						if (*s_Data.TextureSlots[slotIndex] == *texture)
						{
							foundSlotIndex = slotIndex;
							break;
						}
					}

					if (foundSlotIndex != -1)
					{
						indices[textureTypeIndex] = foundSlotIndex;
					}
					else
					{
						if (s_Data.TextureSlotIndex >= Renderer3DData::MaxTextureSlots)
							Renderer3D::NextBatch();

						indices[textureTypeIndex] = s_Data.TextureSlotIndex;
						s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
						s_Data.TextureSlotIndex++;
					}
				}
			}
			return indices;
		}

		glm::mat4 CalculateDirectionalLightSpaceMatrix(
			const glm::mat4 &cameraProjectionMatrix,
			const glm::mat4 &cameraViewMatrix,
			const glm::vec3 &lightDirection,
			float shadowMapDistance,
			float nearPlane,
			float farPlane
		)
		{
			glm::mat4 correctViewProjection = cameraProjectionMatrix * cameraViewMatrix;

			// NDC 范围通常为 [-1, 1] for X, Y, Z
			glm::vec4 frustumCorners[8] = {
				// 近平面 (Near Plane)
				glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),	  // RTN (右上近)
				glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),  // LTN
				glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),  // RBN
				glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), // LBN
				// 远平面 (Far Plane)
				glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),	// RTF (右上远)
				glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), // LTF
				glm::vec4(1.0f, -1.0f, 1.0f, 1.0f), // RBF
				glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f) // LBF
			};

			// --- 2. 变换到世界空间 (World Space) ---
			glm::mat4 invCamVP = glm::inverse(correctViewProjection);
			glm::vec3 center = glm::vec3(0.0f);

			for (int i = 0; i < 8; ++i)
			{
				glm::vec4 worldCorner = invCamVP * frustumCorners[i];
				frustumCorners[i] = worldCorner / worldCorner.w;
				center += glm::vec3(frustumCorners[i]);
			}
			glm::vec3 frustumCorner = center / 8.0f;

			glm::mat4 lightProjection, lightView, lightSpaceMatrix;

			glm::vec3 lightDir = lightDirection;
			glm::vec3 targetPos = frustumCorner;
			glm::vec3 lightPos = targetPos - lightDir * shadowMapDistance;
			glm::vec3 upVector = (glm::abs(glm::dot(lightDir, glm::vec3(0.0, 1.0, 0.0))) > 0.99f)
									 ? glm::vec3(0.0f, 0.0f, 1.0f)
									 : glm::vec3(0.0f, 1.0f, 0.0f);

			lightView = glm::lookAt(lightPos, targetPos, upVector);

			// ------------------------------------------------------------------------

			const float SHADOW_MAP_DIMENSION = 2048.0f; // 确保与你实际的纹理大小一致

			// --- 边界计算 (此部分代码是正确的) ---
			glm::vec3 minLightCoords = glm::vec3(std::numeric_limits<float>::max());
			glm::vec3 maxLightCoords = glm::vec3(std::numeric_limits<float>::lowest());

			for (int i = 0; i < 8; ++i)
			{
				glm::vec4 lightCorner = lightView * frustumCorners[i];
				minLightCoords.x = glm::min(minLightCoords.x, lightCorner.x);
				maxLightCoords.x = glm::max(maxLightCoords.x, lightCorner.x);
				minLightCoords.y = glm::min(minLightCoords.y, lightCorner.y);
				maxLightCoords.y = glm::max(maxLightCoords.y, lightCorner.y);
				minLightCoords.z = glm::min(minLightCoords.z, lightCorner.z);
				maxLightCoords.z = glm::max(maxLightCoords.z, lightCorner.z);
			}

			float lightFrustumWidth = maxLightCoords.x - minLightCoords.x;
			float lightFrustumHeight = maxLightCoords.y - minLightCoords.y;
			float texelSizeX = lightFrustumWidth / SHADOW_MAP_DIMENSION;
			float texelSizeY = lightFrustumHeight / SHADOW_MAP_DIMENSION;

			// 计算对齐前的中心点
			float lightSpaceCenterX = (minLightCoords.x + maxLightCoords.x) * 0.5f;
			float lightSpaceCenterY = (minLightCoords.y + maxLightCoords.y) * 0.5f;

			// 计算对齐后的中心点 (向下取整到最近的纹素网格点)
			float snappedCenterX = floor(lightSpaceCenterX / texelSizeX) * texelSizeX;
			float snappedCenterY = floor(lightSpaceCenterY / texelSizeY) * texelSizeY;

			// 计算平移偏移量
			float translationX = snappedCenterX - lightSpaceCenterX;
			float translationY = snappedCenterY - lightSpaceCenterY;

			glm::mat4 correctionMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(translationX, translationY, 0.0f));

			lightView = correctionMatrix * lightView;

			minLightCoords = glm::vec3(std::numeric_limits<float>::max());
			maxLightCoords = glm::vec3(std::numeric_limits<float>::lowest());
			for (int i = 0; i < 8; ++i)
			{
				glm::vec4 lightCorner = lightView * frustumCorners[i];
				minLightCoords.x = glm::min(minLightCoords.x, lightCorner.x);
				maxLightCoords.x = glm::max(maxLightCoords.x, lightCorner.x);
				minLightCoords.y = glm::min(minLightCoords.y, lightCorner.y);
				maxLightCoords.y = glm::max(maxLightCoords.y, lightCorner.y);
				minLightCoords.z = glm::min(minLightCoords.z, lightCorner.z);
				maxLightCoords.z = glm::max(maxLightCoords.z, lightCorner.z);
			}
			const float Z_OFFSET_NEAR = nearPlane;
			const float Z_OFFSET_FAR = farPlane;

			float lightNearPlane = minLightCoords.z - Z_OFFSET_NEAR;
			float lightFarPlane = maxLightCoords.z + Z_OFFSET_FAR;

			lightProjection = glm::ortho(
				minLightCoords.x, maxLightCoords.x,
				minLightCoords.y, maxLightCoords.y,
				lightNearPlane, lightFarPlane);

			lightSpaceMatrix = lightProjection * lightView;

			return lightSpaceMatrix;
		}

		glm::mat4 CalculateSpotLightSpaceMatrix(
			const glm::vec3 &lightPosition,
			const glm::vec3 &lightDirection,
			float lightCutOffAngleRad,
			float shadowNearPlane,
			float shadowFarPlane)
		{
			glm::vec3 target = lightPosition + lightDirection;

			// b) 定义 Up 向量
			// 使用鲁棒性检查来防止 lightDirection 与世界Y轴平行时 lookAt 失败
			glm::vec3 upVector;
			// 如果光线方向与世界Y轴（0, 1, 0）几乎平行（点积接近1或-1）
			if (std::abs(glm::dot(lightDirection, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f)
			{
				// 垂直光线：使用世界Z轴作为Up向量
				upVector = glm::vec3(0.0f, 0.0f, 1.0f);
			}
			else
			{
				// 非垂直光线：使用世界Y轴作为Up向量
				upVector = glm::vec3(0.0f, 1.0f, 0.0f);
			}

			// Light View Matrix：将世界空间物体转换到以光源为原点，视线对准光线方向的坐标系
			glm::mat4 lightView = glm::lookAt(
				lightPosition, // Eye (光源位置)
				target,		   // Center (看向的目标点)
				upVector	   // Up (向上向量)
			);
			float fovY = lightCutOffAngleRad * 2.0f;
			float aspectRatio = 1.0f;

			// Light Projection Matrix：使用透视投影来模拟聚光灯的光锥
			glm::mat4 lightProjection = glm::perspective(
				fovY,			 // 垂直视野角 (FOV)
				aspectRatio,	 // 宽高比
				shadowNearPlane, // 近裁剪面
				shadowFarPlane	 // 远裁剪面
			);

			// ------------------------------------------------------------------------
			// 3. 最终的光空间矩阵
			// ------------------------------------------------------------------------

			// Light Space Matrix = Light Projection Matrix * Light View Matrix
			glm::mat4 lightSpaceMatrix = lightProjection * lightView;

			return lightSpaceMatrix;
		}
	}

	void Renderer3D::InitSphereGeometry()
	{
		// buffer
		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359f;

		uint32_t SphereVertexCount = (X_SEGMENTS + 1) * (Y_SEGMENTS + 1);
		std::vector<glm::vec3> Sphere_Positions;
		std::vector<glm::vec2> Sphere_TexCoord;
		std::vector<glm::vec3> Sphere_Normals;

		std::vector<glm::vec3> Sphere_Tangents;
		std::vector<glm::vec3> Sphere_Bitangents;

		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;

				// 极坐标到笛卡尔坐标的转换
				float phi = xSegment * 2.0f * PI; // 经度，从 0 到 2PI
				float theta = ySegment * PI;	  // 纬度，从 0 到 PI

				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				glm::vec3 position = glm::vec3(xPos, yPos, zPos);
				glm::vec3 normal = glm::normalize(position); // 对于单位球体，位置向量即为法线

				Sphere_Positions.push_back(position);
				Sphere_Normals.push_back(normal);
				Sphere_TexCoord.push_back(glm::vec2(xSegment, ySegment));

				// 计算切线 (Tangent) 和 副切线 (Bitangent)
				// 切线：通常是沿着U方向（经度方向）的导数
				glm::vec3 tangent;
				if (std::sin(theta) > 0.0001f) // 避免在极点处切线向量为零
					// 对于球体，切线可以简单地计算为 (-z, 0, x)，然后归一化
					// 这对应于沿着经度线方向的切线，在y轴平面上的投影。
					tangent = glm::normalize(glm::vec3(-zPos, 0.0f, xPos));
				else // 在极点处，切线方向不明确，可以给一个默认值，例如(1,0,0)
					tangent = glm::vec3(1.0f, 0.0f, 0.0f);

				// 副切线：法线和切线的叉积
				// 确保副切线与法线和切线正交，并且保持右手法则
				glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

				Sphere_Tangents.push_back(tangent);
				Sphere_Bitangents.push_back(bitangent);
			}
		}

		// 顶点
		std::vector<uint32_t> SphereIndices((Y_SEGMENTS * 2 * (X_SEGMENTS + 1)));

		{
			int k = 0;
			bool oddRow = false;
			for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
			{
				if (!oddRow) // even rows: y == 0, y == 2; and so on
				{
					for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
					{
						SphereIndices[k++] = (y * (X_SEGMENTS + 1) + x);
						SphereIndices[k++] = ((y + 1) * (X_SEGMENTS + 1) + x);
					}
				}
				else
				{
					for (int x = X_SEGMENTS; x >= 0; --x)
					{
						SphereIndices[k++] = ((y + 1) * (X_SEGMENTS + 1) + x);
						SphereIndices[k++] = (y * (X_SEGMENTS + 1) + x);
					}
				}
				oddRow = !oddRow;
			}
		}

		std::vector<Sphere::SphereVertex> sphereVertices;
		sphereVertices.reserve(SphereVertexCount);
		for (size_t i = 0; i < SphereVertexCount; ++i)
		{
			sphereVertices.push_back({Sphere_Positions[i],
									  Sphere_Normals[i],
									  Sphere_TexCoord[i],
									  Sphere_Tangents[i],
									  Sphere_Bitangents[i]});
		}

		// -----------------------------------------------------------------
		// sphere ----------------------------------------------------------

		s_Data.DefaultSphereMesh = Sphere::Create(sphereVertices, SphereIndices);

		// s_Data.SphereVertexArray->UnBind();

		// Clear CPU-side original data as it's now on GPU

		Sphere_Positions.clear();
		Sphere_Normals.clear();
		Sphere_TexCoord.clear();
		Sphere_Tangents.clear();
		Sphere_Bitangents.clear();

		sphereVertices.clear();
		SphereIndices.clear();
	}

	void Renderer3D::Init()
	{
		// Creare Shader
		// -----------------------------------------------------------------
		s_Data.SphereShader = Shader::Create("Assets/shaders/Renderer3D_Sphere.glsl");
		s_Data.ModelShader = Shader::Create("Assets/shaders/Renderer3D_Model.glsl");

		s_Data.PointShadowShader = Shader::Create("Assets/shaders/shadow/Renderer3DPointShadowDepthShader.glsl");

		s_Data.DirectionalShadowShader = Shader::Create("Assets/shaders/shadow/Renderer3DDirectionalShadowDepthShader.glsl");

		s_Data.SpotShadowShader = Shader::Create("Assets/shaders/shadow/Renderer3DSpotShadowDepthShader.glsl");

		// Texture
		// -----------------------------------------------------------------

		int32_t samplers[s_Data.MaxTextureSlots];
		for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
			samplers[i] = i;

		s_Data.SphereShader->Bind();
		s_Data.SphereShader->SetIntArray("u_Textures", samplers, s_Data.MaxTextureSlots);
		s_Data.SphereShader->Unbind();

		s_Data.ModelShader->Bind();
		s_Data.ModelShader->SetIntArray("u_Textures", samplers, s_Data.MaxTextureSlots);
		s_Data.ModelShader->Unbind();

		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		// Point Shadow Texture
		int32_t shadow_offset = 35;

		s_Data.PointShadowTextureStart = shadow_offset;

		int32_t samplerPointShadows[s_Data.MAX_POINT_LIGHTS];
		for (uint32_t i = 0; i < s_Data.MAX_POINT_LIGHTS; i++)
			samplerPointShadows[i] = shadow_offset++;

		s_Data.SphereShader->Bind();
		s_Data.SphereShader->SetIntArray("u_PointShadowDepthMaps", samplerPointShadows, s_Data.MAX_POINT_LIGHTS);
		s_Data.SphereShader->Unbind();

		s_Data.ModelShader->Bind();
		s_Data.ModelShader->SetIntArray("u_PointShadowDepthMaps", samplerPointShadows, s_Data.MAX_POINT_LIGHTS);
		s_Data.ModelShader->Unbind();

		s_Data.DirectionalShadowTextureStart = shadow_offset;

		int32_t samplerDirectionalShadows[s_Data.MAX_DIRECTIONAL_LIGHTS];
		for (uint32_t i = 0; i < s_Data.MAX_DIRECTIONAL_LIGHTS; i++)
		{
			samplerDirectionalShadows[i] = shadow_offset++;
		}

		s_Data.SphereShader->Bind();
		s_Data.SphereShader->SetIntArray("u_DirectionalShadowMaps", samplerDirectionalShadows, s_Data.MAX_DIRECTIONAL_LIGHTS);
		s_Data.SphereShader->Unbind();

		s_Data.ModelShader->Bind();
		s_Data.ModelShader->SetIntArray("u_DirectionalShadowMaps", samplerDirectionalShadows, s_Data.MAX_DIRECTIONAL_LIGHTS);
		s_Data.ModelShader->Unbind();

		s_Data.SpotShadowTextureStart = shadow_offset;

		int32_t samplerSpotShadows[s_Data.MAX_SPOT_LIGHTS];
		for (uint32_t i = 0; i < s_Data.MAX_SPOT_LIGHTS; i++)
		{
			samplerSpotShadows[i] = shadow_offset++;
		}

		s_Data.SphereShader->Bind();
		s_Data.SphereShader->SetIntArray("u_SpotShadowMaps", samplerSpotShadows, s_Data.MAX_SPOT_LIGHTS);
		s_Data.SphereShader->Unbind();

		s_Data.ModelShader->Bind();
		s_Data.ModelShader->SetIntArray("u_SpotShadowMaps", samplerSpotShadows, s_Data.MAX_SPOT_LIGHTS);
		s_Data.ModelShader->Unbind();

		// Add Material 0
		MaterialManager::Get().AddMaterial();

		// -----------------------------------------------------------------
		// Initialize standard sphere geometry (only once)
		InitSphereGeometry();

		// -----------------------------------------------------------------
		// Sphere Instance SSBO
		s_Data.SphereInstanceSSBO = ShaderStorageBuffer::Create(Renderer3DData::MaxSphereCount * sizeof(SphereInstanceData));

		// -----------------------------------------------------------------
		// Light Instance SSBO
		s_Data.LightInstanceSSBO = ShaderStorageBuffer::Create(sizeof(LightData));

		// -----------------------------------------------------------------
		// Point Shadow Instance SSBO
		s_Data.PointShadowInstanceSSBO = ShaderStorageBuffer::Create(Renderer3DData::MAX_SPOT_LIGHTS * sizeof(Renderer3DData::StoredPointShadowGPU));

		// s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::CameraData), 0);

		// -----------------------------------------------------------------
		// Hdr
		s_Data.HdrSkybox = HDRSkybox::Create();
	}

	void Renderer3D::Shutdown()
	{
	}

	void Renderer3D::BeginScene(const EditorCamera &camera)
	{
		s_Data.CameraBuffer.Projection = camera.GetProjection();
		s_Data.CameraBuffer.View = camera.GetViewMatrix();
		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraBuffer.CameraPosition = camera.GetPosition();
		// s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

		StartBatch();
	}

	void Renderer3D::BeginScene(const OrthographicCamera &camera)
	{
		s_Data.CameraBuffer.Projection = camera.GetProjectionMatrix();
		s_Data.CameraBuffer.View = camera.GetViewMatrix();
		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjectionMatrix();
		s_Data.CameraBuffer.CameraPosition = camera.GetPosition();
		// s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

		StartBatch();
	}

	void Renderer3D::BeginScene(const Camera &camera, const glm::mat4 &transform)
	{
		s_Data.CameraBuffer.Projection = camera.GetProjection();
		s_Data.CameraBuffer.View = glm::inverse(transform);
		s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
		s_Data.CameraBuffer.CameraPosition = glm::vec3(transform[3]);
		// s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer3DData::CameraData));

		StartBatch();
	}

	void Renderer3D::EndScene()
	{
		// Light SSBO
		Renderer3D::FlushPointShadows();
		Renderer3D::FlushDirectionalShadows();
		Renderer3D::FlushSpotShadows();
		Renderer3D::FlushLights();

		Flush();

		// clear Light Data
		s_Data.DirectionalLights.clear();
		s_Data.PointLights.clear();
		s_Data.SpotLights.clear();

		// clear shadow Data
		s_Data.PointShadows.clear();
		s_Data.PointShadowTextureSlotIndex = 0;

		s_Data.DirectionalShadows.clear();
		s_Data.DirectionalShadowTextureSlotIndex = 0;

		s_Data.SpotShadows.clear();
		s_Data.SpotShadowTextureSlotIndex = 0;

		// clear obj tranform data
		s_Data.ProjectionShadowDatas.clear();

		// clear hdr
		s_Data.HdrSkybox->Unbind();
		s_Data.HdrSkybox.reset();
	}

	void Renderer3D::StartBatch()
	{
		s_Data.TextureSlotIndex = 1;
		for (uint32_t i = 1; i < Renderer3DData::MaxTextureSlots; ++i) // 从 1 开始清空
		{
			s_Data.TextureSlots[i] = nullptr;
		}

		s_Data.SphereCount = 0;
		s_Data.SphereInstances.clear();

		// 清空所有模型绘制批次
		for (auto &pair : s_Data.ModelDrawCallBatches)
		{
			// 释放SSBO
			pair.second.InstanceSSBO->Destroy();
		}
		s_Data.ModelDrawCallBatches.clear();
		s_Data.ModelCount = 0;
	}

	void Renderer3D::FlushLights()
	{
		LightData data;

		data.CameraPosition = s_Data.CameraBuffer.CameraPosition;
		data.padding1 = 0.0f;

		data.NumDirectionalLights = s_Data.DirectionalLights.size();
		data.NumPointLights = s_Data.PointLights.size();
		data.NumSpotLights = s_Data.SpotLights.size();
		data.padding2 = 0;

		memcpy(data.DirectionalLights, s_Data.DirectionalLights.data(), s_Data.DirectionalLights.size() * sizeof(StoredDirectionalLight));
		memcpy(data.PointLights, s_Data.PointLights.data(), s_Data.PointLights.size() * sizeof(StoredPointLight));
		memcpy(data.SpotLights, s_Data.SpotLights.data(), s_Data.SpotLights.size() * sizeof(StoredSpotLight));

		s_Data.LightInstanceSSBO->Bind();
		s_Data.LightInstanceSSBO->SetData(&data, sizeof(LightData));
		s_Data.LightInstanceSSBO->BindBase(1);
		s_Data.LightInstanceSSBO->UnBind();
	}

    void Renderer3D::FlushPointShadows()
    {
		std::vector<Renderer3DData::StoredPointShadowGPU> gpuDataArray;

		for (auto &data : s_Data.PointShadows)
		{
			gpuDataArray.push_back(data.GPUData);
		}

		s_Data.PointShadowInstanceSSBO->Bind();
		s_Data.PointShadowInstanceSSBO->SetData(gpuDataArray.data(), gpuDataArray.size() * sizeof(Renderer3DData::StoredPointShadowGPU));
		s_Data.PointShadowInstanceSSBO->BindBase(4);
		s_Data.PointShadowInstanceSSBO->UnBind();

		// Bind shadow
		{
			for (auto data : s_Data.PointShadows)
			{
				// --- 状态保存 ---
				GLint viewport[4];
				glGetIntegerv(GL_VIEWPORT, viewport);
				GLint currentFBO;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
				// --- 状态保存结束 ---

				data.Shadow->Bind();
				s_Data.PointShadowShader->Bind();

				// --- 启用并设置深度偏移 ---
				// 启用多边形偏移
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(4.0f, 10.0f);

				s_Data.PointShadowShader->SetInt("u_Slot", data.Slot);

				for (auto &mesh : s_Data.ProjectionShadowDatas)
				{
					s_Data.PointShadowShader->SetMat4("u_Model", mesh.Transform);
					glBindVertexArray(mesh.VertexArrayID);
					glDrawElements(GL_TRIANGLE_STRIP, mesh.IndexCount, GL_UNSIGNED_INT, 0);
				}


				glDisable(GL_POLYGON_OFFSET_FILL);

				data.Shadow->Unbind();
				s_Data.PointShadowShader->Unbind();

				// --- 状态恢复 ---
				glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
				glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
				// --- 状态恢复结束 ---

				data.Shadow->BindTexture(data.Slot + s_Data.PointShadowTextureStart);
			}
		}
	}

    void Renderer3D::FlushDirectionalShadows()
    {
		// Bind shadow
		{
			for (auto data : s_Data.DirectionalShadows)
			{
				// --- 状态保存 ---
				GLint viewport[4];
				glGetIntegerv(GL_VIEWPORT, viewport);
				GLint currentFBO;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
				// --- 状态保存结束 ---

				data.Shadow->Bind();
				s_Data.DirectionalShadowShader->Bind();

				// --- 启用并设置深度偏移 ---
				// 启用多边形偏移
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(4.0f, 10.0f);

				s_Data.DirectionalShadowShader->SetMat4("u_LightSpaceMatrix", data.LightSpaceMatrix);

				for (auto &mesh : s_Data.ProjectionShadowDatas)
				{
					s_Data.DirectionalShadowShader->SetMat4("u_Model", mesh.Transform);
					glBindVertexArray(mesh.VertexArrayID);
					glDrawElements(GL_TRIANGLE_STRIP, mesh.IndexCount, GL_UNSIGNED_INT, 0);
				}

				glDisable(GL_POLYGON_OFFSET_FILL);

				data.Shadow->Unbind();
				s_Data.DirectionalShadowShader->Unbind();

				// --- 状态恢复 ---
				glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
				glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
				// --- 状态恢复结束 ---

				data.Shadow->BindTexture(data.Slot + s_Data.DirectionalShadowTextureStart);
			}
		}
	}

    void Renderer3D::FlushSpotShadows()
    {
		// Bind shadow
		{
			for (auto data : s_Data.SpotShadows)
			{
				// --- 状态保存 ---
				GLint viewport[4];
				glGetIntegerv(GL_VIEWPORT, viewport);
				GLint currentFBO;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
				// --- 状态保存结束 ---

				data.Shadow->Bind();
				s_Data.SpotShadowShader->Bind();

				// --- 启用并设置深度偏移 ---
				// 启用多边形偏移
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(4.0f, 10.0f);

				s_Data.SpotShadowShader->SetMat4("u_LightSpaceMatrix", data.LightSpaceMatrix);

				for (auto &mesh : s_Data.ProjectionShadowDatas)
				{
					s_Data.SpotShadowShader->SetMat4("u_Model", mesh.Transform);
					glBindVertexArray(mesh.VertexArrayID);
					glDrawElements(GL_TRIANGLE_STRIP, mesh.IndexCount, GL_UNSIGNED_INT, 0);
				}

				glDisable(GL_POLYGON_OFFSET_FILL);

				data.Shadow->Unbind();
				s_Data.SpotShadowShader->Unbind();

				// --- 状态恢复 ---
				glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
				glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
				// --- 状态恢复结束 ---

				data.Shadow->BindTexture(data.Slot + s_Data.SpotShadowTextureStart);
			}
		}
	}

    void Renderer3D::Flush()
	{
		// Hdr
		// ------------------------------------
		if (s_Data.HdrSkybox)
		{
			s_Data.HdrSkybox->Render(s_Data.CameraBuffer.Projection, s_Data.CameraBuffer.View);

			s_Data.HdrSkybox->Bind(s_Data.SphereShader);
			s_Data.HdrSkybox->Bind(s_Data.ModelShader);
		}

		// Bind textures
		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->Bind(i);

		// Sphere
		if (s_Data.SphereCount > 0)
		{
			s_Data.SphereShader->Bind();

			s_Data.SphereShader->SetMat4("u_ViewProjection", s_Data.CameraBuffer.ViewProjection);

			s_Data.SphereInstanceSSBO->Bind();
			s_Data.SphereInstanceSSBO->SetData(s_Data.SphereInstances.data(), s_Data.SphereCount * sizeof(SphereInstanceData));
			s_Data.SphereInstanceSSBO->BindBase(2);
			s_Data.SphereInstanceSSBO->UnBind();

			s_Data.DefaultSphereMesh->DrawInstanced(s_Data.SphereShader, s_Data.SphereCount);

			s_Data.SphereShader->Unbind();
		}

		// Model
		{
			s_Data.ModelShader->Bind();

			s_Data.ModelShader->SetMat4("u_ViewProjection", s_Data.CameraBuffer.ViewProjection);

			// 遍历所有收集到的绘制批次
			for (auto &pair : s_Data.ModelDrawCallBatches)
			{
				Renderer3DData::ModelCallKey key = pair.first;
				Renderer3DData::ModelCallBatch &batch = pair.second;

				if (batch.InstanceCount > 0)
				{
					// 上传实例数据到 SSBO
					batch.InstanceSSBO->Bind();
					batch.InstanceSSBO->SetData(batch.Instances.data(), (uint32_t)batch.InstanceCount * sizeof(MeshInstanceData));
					batch.InstanceSSBO->BindBase(3);
					batch.InstanceSSBO->UnBind();

					Ref<Mesh> mesh = MeshManager::Get().GetMeshByID(key.MeshID);
					mesh->DrawInstanced(s_Data.ModelShader, batch.InstanceCount);

					s_Data.Stats.MeshCount++;
				}
			}

			s_Data.ModelShader->Unbind();
		}

		// unbind
		for (auto &pair : s_Data.ModelDrawCallBatches)
		{
			pair.second.Instances.clear();
			pair.second.InstanceCount = 0;
		}

		s_Data.Stats.DrawCalls++;
	}

	void Renderer3D::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer3D::DrawSphere(const glm::mat4 &transform, SphereRendererComponent &src, MaterialComponent *material, int entityID)
	{
		if (s_Data.SphereCount >= Renderer3DData::MaxSphereCount)
		{
			NextBatch();
		}

		if (src.ProjectionShadow)
		{
			auto id = s_Data.DefaultSphereMesh->GetVertexArray()->GetRendererID();
			auto count = s_Data.DefaultSphereMesh->GetVertexArray()->GetIndexBuffer()->GetCount();
			s_Data.ProjectionShadowDatas.push_back({
				transform,
				id,
				count
			});
		}

		glm::vec4 albedo = glm::vec4(1.0f);
		glm::vec4 emissive = glm::vec4(0.0f);

		std::vector<int> arrIndex(7, 0);

		float roughness = 0.5f;
		float metallic = 0.0f;
		float ao = 1.0f;

		if (material != nullptr)
		{	
			arrIndex = Mc::Utils::GetMaterialTextureIndices(*material);
			albedo = glm::vec4(material->Albedo, 1.0f);
			emissive = glm::vec4(material->Emissive, 1.0f);
			roughness = material->Roughness;
			metallic = material->Metallic;
			ao = material->Ao;
		}

		s_Data.SphereInstances.push_back({
			transform,
			src.Color,

			albedo,
			emissive,
			roughness,
			metallic,
			ao,

			// Texture
			arrIndex[0],
			arrIndex[1],
			arrIndex[2],
			arrIndex[3],
			arrIndex[4],
			arrIndex[5],
			arrIndex[6],

			src.TilingFactor,
			entityID,

			(int)src.ReceivesPBR,
			(int)src.ReceivesIBL,
			(int)src.ReceivesLight,
			(int)src.ReceivesShadow,
		});

		arrIndex.clear();
		s_Data.SphereCount++;
		s_Data.Stats.SphereCount++;
	}

    void Renderer3D::DrawModel(const glm::mat4 &transform, ModelRendererComponent &src, MeshRendererComponent *mesh, MaterialComponent *material, int entityID)
    {
		if (src.ModelPath.empty())
			return;

		// 创建一个用于哈希表的键
		Renderer3DData::ModelCallKey key;
		key.MeshID = mesh->Id;

		Renderer3DData::ModelCallBatch &batch = s_Data.ModelDrawCallBatches[key];

		if (batch.InstanceSSBO == nullptr)
		{
			batch.InstanceSSBO = ShaderStorageBuffer::Create(Renderer3DData::MaxInstancesPerModel * sizeof(MeshInstanceData));
			batch.Instances.reserve(Renderer3DData::MaxInstancesPerModel);
		}

		if (batch.InstanceCount >= Renderer3DData::MaxInstancesPerModel)
		{
			NextBatch();
		}

		if (src.ProjectionShadow)
		{
			auto data = MeshManager::Get().GetMeshByID(mesh->Id);
			auto id = data->GetVertexArray()->GetRendererID();
			auto count = data->GetIndexBuffer()->GetCount();

			s_Data.ProjectionShadowDatas.push_back({
				transform,
				id,
				count,
			});
		}

		glm::vec4 albedo = glm::vec4(1.0f);
		glm::vec4 emissive = glm::vec4(0.0f);

		std::vector<int> arrIndex(7, 0);

		float roughness = 0.5f;
		float metallic = 0.0f;
		float ao = 1.0f;

		if (material != nullptr)
		{
			arrIndex = Mc::Utils::GetMaterialTextureIndices(*material);
			albedo = glm::vec4(material->Albedo, 1.0f);
			emissive = glm::vec4(material->Emissive, 1.0f);
			roughness = material->Roughness;
			metallic = material->Metallic;
			ao = material->Ao;
		}

		// 填充实例数据
		MeshInstanceData instanceData = {
			transform,
			src.Color,

			albedo,
			emissive,
			roughness,
			metallic,
			ao,

			// Texture
			arrIndex[0],
			arrIndex[1],
			arrIndex[2],
			arrIndex[3],
			arrIndex[4],
			arrIndex[5],
			arrIndex[6],

			entityID,
			(int)src.FlipUV,

			(int)src.ReceivesPBR,
			(int)src.ReceivesIBL,
			(int)src.ReceivesLight,
			(int)src.ReceivesShadow,
		};

		batch.Instances.push_back(instanceData);
		batch.InstanceCount++;

		arrIndex.clear();

		s_Data.ModelCount++;
		s_Data.Stats.ModelCount++; // 统计渲染的模型实例数量
	}

    void Renderer3D::DrawDirectionalLight(const glm::mat4 &transform, DirectionalLightComponent &src, ShadowComponent *shadow, int entityID)
	{
		if (s_Data.DirectionalLights.size() < Renderer3DData::MAX_DIRECTIONAL_LIGHTS)
		{
			glm::vec3 direction = glm::normalize(glm::vec3(transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

			StoredDirectionalLight dirLight;
			dirLight.Direction = direction;
			dirLight.Color = src.Color;
			dirLight.Intensity = src.Intensity;
			dirLight.CastsShadows = (int)src.CastsShadows;

			dirLight.padding_0 = 0.0f;
			dirLight.padding_1 = 0.0f;
			dirLight.padding_2 = 0.0f;

			auto it = s_Data.DirectionalShadowMaps.find(entityID);
			if (it != s_Data.DirectionalShadowMaps.end())
			{
				const glm::mat4 &currentProjection = s_Data.CameraBuffer.Projection;
				const glm::mat4 &currentViewMatrix = s_Data.CameraBuffer.View;

				glm::mat4 lightSpaceMatrix =  Utils::CalculateDirectionalLightSpaceMatrix(currentProjection, currentViewMatrix, direction, 10.0f, shadow->NearPlane, shadow->FarPlane);
				// ================================================

				dirLight.LightSpaceMatrix = lightSpaceMatrix;
				dirLight.ShadowMapIndex = s_Data.DirectionalShadowTextureSlotIndex;

				Renderer3DData::StoredDirectionalShadowCPU data;

				data.LightSpaceMatrix = lightSpaceMatrix;
				data.Shadow = it->second;
				data.Slot = s_Data.DirectionalShadowTextureSlotIndex;

				s_Data.DirectionalShadows.push_back(data);
				s_Data.DirectionalShadowTextureSlotIndex++;
			}
			else
			{
				dirLight.LightSpaceMatrix = glm::mat4(0.0);
				dirLight.ShadowMapIndex = -1;
			}

			s_Data.DirectionalLights.push_back(dirLight);
		}
		else
		{
			LOG_CORE_WARN("Max directional lights reached!");
		}
	}

	void Renderer3D::DrawPointLight(const glm::mat4 &transform, PointLightComponent &src, ShadowComponent *shadow, int entityID)
	{
		if (s_Data.PointLights.size() < Renderer3DData::MAX_POINT_LIGHTS)
		{
			glm::vec3 position = transform[3];
			glm::vec3 direction = glm::normalize(glm::vec3(transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

			StoredPointLight ptLight;

			ptLight.Position = position;
			ptLight.Color = src.Color;
			ptLight.Intensity = src.Intensity;
			ptLight.Radius = src.Radius;
			ptLight.CastsShadows = (int)src.CastsShadows;

			auto it = s_Data.PointShadowMaps.find(entityID);

			if (it != s_Data.PointShadowMaps.end())
			{
				ptLight.FarPlane = shadow->FarPlane;
				ptLight.ShadowMapIndex = s_Data.PointShadowTextureSlotIndex;
				ptLight.padding_0 = 0.0f;

				Renderer3DData::StoredPointShadowCPU data;
				Renderer3DData::StoredPointShadowGPU gpu_data;

				gpu_data.LightPositionFarPlane.x = position.x;
				gpu_data.LightPositionFarPlane.y = position.y;
				gpu_data.LightPositionFarPlane.z = position.z;
				gpu_data.LightPositionFarPlane.w = shadow->FarPlane;

				glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)shadow->Resolution / (float)shadow->Resolution, shadow->NearPlane, shadow->FarPlane);
				gpu_data.ShadowTransforms[0] = shadowProj * glm::lookAt(position, position + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
				gpu_data.ShadowTransforms[1] = shadowProj * glm::lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
				gpu_data.ShadowTransforms[2] = shadowProj * glm::lookAt(position, position + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
				gpu_data.ShadowTransforms[3] = shadowProj * glm::lookAt(position, position + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
				gpu_data.ShadowTransforms[4] = shadowProj * glm::lookAt(position, position + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
				gpu_data.ShadowTransforms[5] = shadowProj * glm::lookAt(position, position + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));

				data.GPUData = gpu_data;
				data.Shadow = it->second;
				data.Slot = s_Data.PointShadowTextureSlotIndex;

				s_Data.PointShadows.push_back(data);
				s_Data.PointShadowTextureSlotIndex++;
			}
			else
			{
				ptLight.FarPlane = 0.0f;
				ptLight.ShadowMapIndex = -1;
				ptLight.padding_0 = 0.0f;
			}
			s_Data.PointLights.push_back(ptLight);
		}
		else
		{
			LOG_CORE_WARN("Max point lights reached!");
		}
	}

	void Renderer3D::DrawSpotLight(const glm::mat4 &transform, SpotLightComponent &src, ShadowComponent *shadow, int entityID)
	{
		if (s_Data.SpotLights.size() < Renderer3DData::MAX_SPOT_LIGHTS)
		{
			glm::vec3 position = transform[3];
			glm::vec3 direction = glm::normalize(glm::vec3(transform * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

			StoredSpotLight spLight;
			spLight.Position = position;
			spLight.Direction = direction; // 聚光灯方向
			spLight.Color = src.Color;
			spLight.Intensity = src.Intensity;
			spLight.Radius = src.Radius;

			spLight.InnerConeCos = glm::cos(src.InnerConeAngle); // GLSL 内边界使用外锥角
			spLight.OuterConeCos = glm::cos(src.OuterConeAngle); // GLSL 外边界使用内锥角

			spLight.CastsShadows = (int)src.CastsShadows;
			spLight.padding_0 = 0.0f;

			auto it = s_Data.SpotShadowMaps.find(entityID);
			if (it != s_Data.SpotShadowMaps.end())
			{
				glm::mat4 lightSpaceMatrix = Utils::CalculateSpotLightSpaceMatrix(position, direction, src.OuterConeAngle, shadow->NearPlane, shadow->FarPlane);
				// ================================================

				spLight.LightSpaceMatrix = lightSpaceMatrix;
				spLight.ShadowMapIndex = s_Data.SpotShadowTextureSlotIndex;

				Renderer3DData::StoredSpotShadowCPU data;

				data.LightSpaceMatrix = lightSpaceMatrix;
				data.Shadow = it->second;
				data.Slot = s_Data.SpotShadowTextureSlotIndex;

				s_Data.SpotShadows.push_back(data);
				s_Data.SpotShadowTextureSlotIndex++;
			}
			else
			{
				spLight.LightSpaceMatrix = glm::mat4(0.0);
				spLight.ShadowMapIndex = -1;
			}
			s_Data.SpotLights.push_back(spLight);
		}
		else
		{
			LOG_CORE_WARN("Max spot lights reached!");
		}
	}

    void Renderer3D::AddDirectionalShadow(entt::entity entity, unsigned int resolution)
    {
		auto it = s_Data.DirectionalShadowMaps.find((int)entity);
		if (it == s_Data.DirectionalShadowMaps.end())
			s_Data.DirectionalShadowMaps[(int)entity] = DirectionalShadowMap::Create(resolution);
	}

    void Renderer3D::DelDirectionalShadow(entt::entity entity)
    {
		auto it = s_Data.DirectionalShadowMaps.find((int)entity);
		if (it != s_Data.DirectionalShadowMaps.end())
			s_Data.DirectionalShadowMaps.erase(it);
	}

	void Renderer3D::AddPointShadow(entt::entity entity, unsigned int resolution)
	{
		auto it = s_Data.PointShadowMaps.find((int)entity);
		if (it == s_Data.PointShadowMaps.end())
			s_Data.PointShadowMaps[(int)entity] = PointShadowMap::Create(resolution);
	}

	void Renderer3D::DelPointShadow(entt::entity entity)
	{
		auto it = s_Data.PointShadowMaps.find((int)entity);
		if (it != s_Data.PointShadowMaps.end())
			s_Data.PointShadowMaps.erase(it);
	}

    void Renderer3D::AddSpotShadow(entt::entity entity, unsigned int resolution)
    {
		auto it = s_Data.SpotShadowMaps.find((int)entity);
		if (it == s_Data.SpotShadowMaps.end())
			s_Data.SpotShadowMaps[(int)entity] = SpotShadowMap::Create(resolution);
	}

	void Renderer3D::DelSpotShadow(entt::entity entity)
	{
		auto it = s_Data.SpotShadowMaps.find((int)entity);
		if (it != s_Data.SpotShadowMaps.end())
			s_Data.SpotShadowMaps.erase(it);
	}

    void Renderer3D::DrawHdrSkybox(HdrSkyboxComponent &src, int entityID)
	{
		HdrManager::Get().SetActive(src.Path);
		s_Data.HdrSkybox = HdrManager::Get().GetActive();
	}

	void Renderer3D::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer3D::Statistics Renderer3D::GetStats()
	{
		return s_Data.Stats;
	}
}