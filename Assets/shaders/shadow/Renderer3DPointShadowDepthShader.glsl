#type vertex
#version 450 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

uniform mat4 u_Model;
uniform mat4 u_FinalBoneMatrices[MAX_BONES];

out VS_OUT {
    vec4 WorldPos;
} vs_out;

void main()
{
    mat4 boneTransform = mat4(0.0f);
    float totalWeight = 0.0;
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        if (a_BoneIDs[i] == -1) continue;
        if (a_BoneIDs[i] >= MAX_BONES) break;

        boneTransform += u_FinalBoneMatrices[a_BoneIDs[i]] * a_Weights[i];
        totalWeight += a_Weights[i];
    }
    // 如果没有权重（静态物体），设为单位矩阵
    if (totalWeight < 0.01) boneTransform = mat4(1.0);

    vec4 localAnimatedPos = boneTransform * vec4(a_Position, 1.0);

    vs_out.WorldPos = u_Model * localAnimatedPos;
}

#type geometry
#version 450 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

in VS_OUT {
    vec4 WorldPos;
} gs_in[];

out vec4 FragPos;
out vec3 LightPosition;
out float FarPlane;

struct StoredPointShadowGPU
{
    mat4 ShadowTransforms[6];
    vec4 LightPositionFarPlane;
};

layout(std140, binding = 4) readonly buffer PointShadowBuffer {
    StoredPointShadowGPU shadows[];
};

uniform int u_Slot;

void main()
{
    StoredPointShadowGPU data = shadows[u_Slot];

    LightPosition = data.LightPositionFarPlane.xyz;
    FarPlane = data.LightPositionFarPlane.w;

    for(int face = 0; face < 6; face++)
    {
        gl_Layer = face;
        
        for(int i = 0; i < 3; ++i)
        {
            vec4 worldPos = gs_in[i].WorldPos;
            FragPos = worldPos;
            gl_Position = data.ShadowTransforms[face] * worldPos;
            
            EmitVertex();
        }
        EndPrimitive();
    }
}

#type fragment
#version 450 core

in vec4 FragPos;
in vec3 LightPosition;
in float FarPlane;

void main()
{
    float lightDistance = length(FragPos.xyz - LightPosition);
    lightDistance = lightDistance / FarPlane;
    gl_FragDepth = lightDistance;
}