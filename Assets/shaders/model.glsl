// Basic Texure Shader

#type vertex
#version 450 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;

struct InstanceData {
    mat4 Transform;
    vec4 TintColor;

	// PBR
	vec4 AlbedoColor;
	vec4 EmissiveColor;
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
	int padding_0;
};

// SSBO Declaration
layout(std140, binding = 3) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

// Output to fragment shader
out VS_OUT {
    vec3 WorldPos;    // 世界空间位置
    vec3 NormalWS;    // 世界空间法线 (未归一化)
    mat3 TBN;         // 切线空间到世界空间的转换矩阵
    vec2 TexCoords;
    vec4 TintColor;

    // PBR
	vec4 AlbedoColor;
    vec4 EmissiveColor;

	float Roughness;
	float Metallic;
	float AO; // 环境光遮蔽

	// 纹理贴图 (对应 Material.h 中的 TextureType 枚举)
	flat int AlbedoMapIndex;	   // 0
	flat int NormalMapIndex;	   // 1
	flat int MetallicMapIndex;     // 2
	flat int RoughnessMapIndex;    // 3
	flat int AoMapIndex;		   // 4
	flat int EmissiveMapIndex;     // 5
	flat int HeightMapIndex;	   // 6

    flat int EntityID;
    flat int FlipUV;

    flat int ReceivesPBR;
	flat int ReceivesIBL;
	flat int ReceivesLight;
} vs_out;


uniform mat4 u_ViewProjection;

void main()
{
    InstanceData instance = instances[gl_InstanceID];

    mat4 transform  = instance.Transform;

    vec4 worldPos4 = transform * vec4(a_Position, 1.0);
    vs_out.WorldPos = worldPos4.xyz;     // 世界空间位置
    gl_Position = u_ViewProjection * worldPos4;

    // 世界空间法线 (为了正确变换法线，需要使用模型矩阵的逆转置矩阵)
    // 假设没有非统一缩放，可以直接用 mat3(Model)
    vs_out.NormalWS = mat3(transpose(inverse(transform))) * a_Normal;

    vs_out.TexCoords = a_TexCoords;
    vs_out.TintColor = instance.TintColor;

    // 计算 TBN 矩阵 (切线空间到世界空间)
    vec3 T = normalize(mat3(transform) * a_Tangent);
    vec3 B = normalize(mat3(transform) * a_Bitangent);
    vec3 N = normalize(mat3(transform) * a_Normal);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));
    vs_out.TBN = mat3(T, B, N);

    // PBR
	vs_out.AlbedoColor = instance.AlbedoColor;
	vs_out.EmissiveColor = instance.EmissiveColor;
	vs_out.Roughness = instance.Roughness;
	vs_out.Metallic = instance.Metallic;
	vs_out.AO = instance.AO;

	vs_out.AlbedoMapIndex = instance.AlbedoMapIndex;
	vs_out.NormalMapIndex = instance.NormalMapIndex;
	vs_out.MetallicMapIndex = instance.MetallicMapIndex;
	vs_out.RoughnessMapIndex = instance.RoughnessMapIndex;
	vs_out.AoMapIndex = instance.AoMapIndex;
	vs_out.EmissiveMapIndex = instance.EmissiveMapIndex;
	vs_out.HeightMapIndex = instance.HeightMapIndex;

    vs_out.EntityID = instance.EntityID;
    vs_out.FlipUV = instance.FlipUV;

    vs_out.ReceivesPBR = instance.ReceivesPBR;
    vs_out.ReceivesIBL = instance.ReceivesIBL;
    vs_out.ReceivesLight = instance.ReceivesLight;
}

// ======================================================================

#type fragment
#version 450 core
layout (location = 0) out vec4 o_Color;
layout (location = 1) out int o_EntityID;

uniform sampler2D u_Textures[32];

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

in VS_OUT {
    vec3 WorldPos;    // 世界空间位置
    vec3 NormalWS;    // 世界空间法线 (未归一化)
    mat3 TBN;         // 切线空间到世界空间的转换矩阵
    vec2 TexCoords;
    vec4 TintColor;

    // PBR
	vec4 AlbedoColor;
    vec4 EmissiveColor;

	float Roughness;
	float Metallic;
	float AO; // 环境光遮蔽

	// 纹理贴图 (对应 Material.h 中的 TextureType 枚举)
	flat int AlbedoMapIndex;	   // 0
	flat int NormalMapIndex;	   // 1
	flat int MetallicMapIndex;     // 2
	flat int RoughnessMapIndex;    // 3
	flat int AoMapIndex;		   // 4
	flat int EmissiveMapIndex;     // 5
	flat int HeightMapIndex;	   // 6

    flat int EntityID;
    flat int FlipUV;

    flat int ReceivesPBR;
	flat int ReceivesIBL;
	flat int ReceivesLight;
} fs_in;

// ===================================================================================================================
// 光源 Uniforms (与 C++ Renderer3DData 中的定义一致)
#define MAXLIGHTS 8
#define MAX_DIRECTIONAL_LIGHTS MAXLIGHTS
#define MAX_POINT_LIGHTS       MAXLIGHTS
#define MAX_SPOT_LIGHTS        MAXLIGHTS

struct DirectionalLight {
    vec3 direction; // 光源方向 (通常是从物体指向光源，或约定为光线射来的方向的反方向)
    float intensity;
    vec3 color;
    float pad;
};

struct PointLight {
    vec3 position; // 世界空间位置
    float intensity;
    vec3 color;
    float radius; // 影响半径
};

struct SpotLight {
    vec3 position;  // 世界空间位置
    float intensity;
    vec3 color;
    float radius;
    vec3 direction; // 世界空间方向 (光源自身指向的方向)
    float innerConeCos; // 内锥角的余弦值 (cos(OuterConeAngle))
    vec3 pad;
    float outerConeCos; // 外锥角的余弦值 (cos(InnerConeAngle))
};

// SSBO Light
layout(std430, binding = 1) readonly buffer LightData {
    vec3 CameraPosition;
    float padding1;

    int NumDirectionalLights;
    int NumPointLights;
    int NumSpotLights;
    int padding2;
    
    DirectionalLight DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight PointLights[MAX_POINT_LIGHTS];
    SpotLight SpotLights[MAX_SPOT_LIGHTS];
};

// ----------------------------------------------------------------------------
// PBR Helper Functions (Cook-Torrance BRDF)
// ----------------------------------------------------------------------------

const float PI = 3.14159265359;

// Normal Distribution Function (Trowbridge-Reitz GGX)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0; // for direct lighting

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}
// Fresnel Schlick's approximation
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


void main()
{
    o_EntityID = fs_in.EntityID;

    vec2 finalTexCoords = fs_in.TexCoords;
    if (fs_in.FlipUV == 1) {
        finalTexCoords.y = 1.0 - finalTexCoords.y;
    }

    // 基础颜色 / 反射率 (Albedo)
    vec3 albedo = fs_in.AlbedoColor.rgb * fs_in.TintColor.rgb;
    if (fs_in.AlbedoMapIndex > -1) {
        albedo *= pow(texture(u_Textures[fs_in.AlbedoMapIndex], finalTexCoords).rgb, vec3(2.2));
    }
    float alpha = fs_in.AlbedoColor.a * fs_in.TintColor.a;

    // 粗糙度：结合贴图和实例数据
    float roughness = fs_in.Roughness;
    if (fs_in.RoughnessMapIndex > -1) {
        roughness *= texture(u_Textures[fs_in.RoughnessMapIndex], finalTexCoords).r;
    }
    
    // 金属度：结合贴图和实例数据
    float metallic = fs_in.Metallic;
    if (fs_in.MetallicMapIndex > -1) {
        metallic *= texture(u_Textures[fs_in.MetallicMapIndex], finalTexCoords).r;
    }

    // 环境光遮蔽 (AO)：结合贴图和实例数据
    float ao = fs_in.AO;
    if (fs_in.AoMapIndex > -1) {
        ao *= texture(u_Textures[fs_in.AoMapIndex], finalTexCoords).r;
    }

    // 自发光：结合贴图和实例数据
    vec3 emissive = fs_in.EmissiveColor.rgb;
    if (fs_in.EmissiveMapIndex > -1) {
        emissive *= texture(u_Textures[fs_in.EmissiveMapIndex], finalTexCoords).rgb;
    }

    vec3 finalColor;

    if (fs_in.ReceivesLight == 1)
    {
        // --- 法线贴图计算 ---

        vec3 N = normalize(fs_in.NormalWS); // 默认使用世界空间法线
        if (fs_in.NormalMapIndex > -1) {
            // 假设法线贴图存储在 (0, 1) 范围，需要转换到 (-1, 1)
            vec3 normalFromMap = texture(u_Textures[fs_in.NormalMapIndex], finalTexCoords).rgb;
            normalFromMap = normalFromMap * 2.0 - 1.0;
            // 将法线从切线空间转换到世界空间
            N = normalize(fs_in.TBN * normalFromMap); 
        }
        // --- 遍历所有光源并累加贡献 ---
        vec3 Lo = vec3(0.0); // 累加所有直接光照
        vec3 ambient = 0.03 * albedo * ao;

        if (fs_in.ReceivesPBR == 1)
        {
            // --- PBR 核心计算参数 ---

            // 视图方向 (从片元到相机)
            vec3 V = normalize(CameraPosition - fs_in.WorldPos);

            // 菲涅尔反射率 F0 (基础反射率)
            // 对于非金属，F0 通常是 0.04。对于金属，F0 等于 Albedo。
            vec3 F0 = vec3(0.04);
            F0 = mix(F0, albedo, metallic);

            
            // ------------------------------------
            // --- 光源循环：计算 PBR 直接光照 ---
            // ------------------------------------

            // 定向光
            for (int i = 0; i < NumDirectionalLights; ++i)
            {
                DirectionalLight light = DirectionalLights[i];

                // 光源方向 (从片元到光源)
                vec3 L = normalize(-light.direction);
                // 半向量 (Halfway Vector)
                vec3 H = normalize(V + L);

                // NdotL、NdotV、NdotH、HdotV
                float NdotL = max(dot(N, L), 0.0);
                float NdotV = max(dot(N, V), 0.0);
                float NdotH = max(dot(N, H), 0.0);
                float HdotV = max(dot(H, V), 0.0);

                if (NdotL > 0.0) // 确保光线在法线正面
                {
                    // PBR 核心计算
                    float D = DistributionGGX(N, H, roughness);
                    float G = GeometrySmith(N, V, L, roughness);
                    vec3 F = FresnelSchlick(HdotV, F0);

                    // 分母部分，防止除零
                    vec3 nominator = D * G * F;
                    float denominator = 4.0 * NdotV * NdotL + 0.001;
                    vec3 specular = nominator / denominator;

                    // F_diffuse = (1 - F) * (1 - metallic)
                    vec3 kD = F;
                    kD = vec3(1.0) - kD;
                    kD *= 1.0 - metallic;

                    // 光照贡献
                    vec3 radiance = light.color * light.intensity;
                    vec3 directLightContribution = (kD * albedo / PI + specular) * radiance * NdotL;

                    Lo += directLightContribution;
                }
            }

            // 点光源
            for (int i = 0; i < NumPointLights; ++i)
            {
                PointLight light = PointLights[i];

                vec3 lightToPixel = light.position - fs_in.WorldPos;
                float distance = length(lightToPixel);
                vec3 L = normalize(lightToPixel);
                
                // 光照衰减
                float attenuation = clamp(1.0 - distance / light.radius, 0.0, 1.0);
                if (attenuation <= 0.0) continue;

                vec3 H = normalize(V + L);

                float NdotL = max(dot(N, L), 0.0);
                float NdotV = max(dot(N, V), 0.0);
                float NdotH = max(dot(N, H), 0.0);
                float HdotV = max(dot(H, V), 0.0);

                if (NdotL > 0.0)
                {
                    float D = DistributionGGX(N, H, roughness);
                    float G = GeometrySmith(N, V, L, roughness);
                    vec3 F = FresnelSchlick(HdotV, F0);

                    vec3 nominator = D * G * F;
                    float denominator = 4.0 * NdotV * NdotL + 0.001;
                    vec3 specular = nominator / denominator;

                    vec3 kD = F;
                    kD = vec3(1.0) - kD;
                    kD *= 1.0 - metallic;
                    
                    vec3 radiance = light.color * light.intensity;
                    vec3 directLightContribution = (kD * albedo / PI + specular) * radiance * NdotL * attenuation;
                    
                    Lo += directLightContribution;
                }
            }
            
            // 聚光灯
            for (int i = 0; i < NumSpotLights; ++i)
            {
                SpotLight light = SpotLights[i];
                
                vec3 lightToPixel = light.position - fs_in.WorldPos;
                float distance = length(lightToPixel);
                vec3 L = normalize(lightToPixel);
                
                float attenuation = clamp(1.0 - distance / light.radius, 0.0, 1.0);
                if (attenuation <= 0.0) continue;

                float theta = dot(L, normalize(-light.direction));
                float epsilon = light.innerConeCos - light.outerConeCos;
                float spotFactor = clamp((theta - light.outerConeCos) / epsilon, 0.0, 1.0);
                if (spotFactor <= 0.0) continue;

                vec3 H = normalize(V + L);
                
                float NdotL = max(dot(N, L), 0.0);
                float NdotV = max(dot(N, V), 0.0);
                float NdotH = max(dot(N, H), 0.0);
                float HdotV = max(dot(H, V), 0.0);
                
                if (NdotL > 0.0)
                {
                    float D = DistributionGGX(N, H, roughness);
                    float G = GeometrySmith(N, V, L, roughness);
                    vec3 F = FresnelSchlick(HdotV, F0);

                    vec3 nominator = D * G * F;
                    float denominator = 4.0 * NdotV * NdotL + 0.001;
                    vec3 specular = nominator / denominator;

                    vec3 kD = F;
                    kD = vec3(1.0) - kD;
                    kD *= 1.0 - metallic;

                    vec3 radiance = light.color * light.intensity;
                    vec3 directLightContribution = (kD * albedo / PI + specular) * radiance * NdotL * attenuation * spotFactor;

                    Lo += directLightContribution;
                }
            }

            if (fs_in.ReceivesIBL == 1)
            {
                // IBL
                vec3 R = reflect(-V, N);

                vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
                
                vec3 kS = F;
                vec3 kD = 1.0 - kS;
                kD *= 1.0 - metallic;	  
                
                vec3 irradiance = texture(irradianceMap, N).rgb;
                vec3 diffuse      = irradiance * albedo;
                
                // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
                const float MAX_REFLECTION_LOD = 4.0;
                vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
                vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
                vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

                ambient = (kD * diffuse + specular) * ao;
            }
        }
        else // 非 PBR 模式：使用简单的光照
        {
            vec3 V = normalize(CameraPosition - fs_in.WorldPos);
            
            // Lambertian 漫反射 + 简单环境光
            for (int i = 0; i < NumDirectionalLights; ++i)
            {
                DirectionalLight light = DirectionalLights[i];
                vec3 L = normalize(-light.direction);
                float NdotL = max(dot(N, L), 0.0);
                
                Lo += albedo * light.color * light.intensity * NdotL;
            }
            for (int i = 0; i < NumPointLights; ++i)
            {
                PointLight light = PointLights[i];
                vec3 L = normalize(light.position - fs_in.WorldPos);
                float NdotL = max(dot(N, L), 0.0);
                
                float attenuation = clamp(1.0 - length(light.position - fs_in.WorldPos) / light.radius, 0.0, 1.0);
                if (attenuation <= 0.0) continue;

                Lo += albedo * light.color * light.intensity * NdotL * attenuation;
            }
            for (int i = 0; i < NumSpotLights; ++i)
            {
                SpotLight light = SpotLights[i];
                vec3 L = normalize(light.position - fs_in.WorldPos);
                float NdotL = max(dot(N, L), 0.0);
                
                float attenuation = clamp(1.0 - length(light.position - fs_in.WorldPos) / light.radius, 0.0, 1.0);
                if (attenuation <= 0.0) continue;

                float theta = dot(L, normalize(-light.direction));
                float epsilon = light.innerConeCos - light.outerConeCos;
                float spotFactor = clamp((theta - light.outerConeCos) / epsilon, 0.0, 1.0);
                if (spotFactor <= 0.0) continue;

                Lo += albedo * light.color * light.intensity * NdotL * attenuation * spotFactor;
            }
            
            ambient = 0.03 * albedo * ao;
        }
        
        finalColor = ambient + Lo + emissive;
    }
    else // 不接收任何光照
    {
        finalColor = albedo + emissive;
    }

    // HDR tonemapping
    finalColor = finalColor / (finalColor + vec3(1.0));
    // gamma correct
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    o_Color = vec4(finalColor, alpha);

    // o_Color = vec4(albedo, 1.0);
    // o_Color = vec4(metallic, 1.0, 1.0, 1.0);
    // o_Color = vec4(roughness, 1.0, 1.0, 1.0);
    // o_Color = vec4(ao, 1.0, 1.0, 1.0);
}