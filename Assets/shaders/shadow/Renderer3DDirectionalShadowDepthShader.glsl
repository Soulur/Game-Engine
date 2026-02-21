#type vertex
#version 450 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 5) in ivec4 a_BoneIDs;
layout (location = 6) in vec4 a_Weights;

#define MAX_BONES 100
#define MAX_BONE_INFLUENCE 4

uniform mat4 u_LightSpaceMatrix; 
uniform mat4 u_Model;
uniform mat4 u_FinalBoneMatrices[MAX_BONES];

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

    vec4 worldPos4 = u_Model * localAnimatedPos;
    gl_Position = u_LightSpaceMatrix * worldPos4;
}

#type fragment
#version 450 core
out vec4 FragColor;

void main()
{
}