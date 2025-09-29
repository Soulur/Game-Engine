#type vertex
#version 450 core
layout (location = 0) in vec3 a_Position;

uniform mat4 u_Model;

out VS_OUT {
    vec4 WorldPos;
} vs_out;

void main()
{
    vs_out.WorldPos = u_Model * vec4(a_Position, 1.0f);
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