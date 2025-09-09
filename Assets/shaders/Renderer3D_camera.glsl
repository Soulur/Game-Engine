// Basic Texure Shader

#type vertex
#version 330 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec2 a_TexCoords;
layout (location = 2) in float a_TexIndex;
layout (location = 3) in int a_EntityID;

// layout(std140, binding = 0) uniform Camera
// {
//     mat4 u_ViewProjection;
// };

out vec2 v_TexCoords;
out float v_TexIndex;
flat out int v_EntityID;

uniform mat4 u_ViewProjection;

void main()
{
    v_TexCoords = a_TexCoords;
    v_TexIndex = a_TexIndex;
	v_EntityID = a_EntityID;

    gl_Position =  u_ViewProjection * vec4(a_Position, 1.0);
}
// ===================================================================================================================
#type fragment
#version 330 core
layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

in vec2 v_TexCoords;
in float v_TexIndex;
flat in int v_EntityID;

uniform sampler2D u_Textures[32];

// ----------------------------------------------------------------------------
void main()
{
    texColor *= texture(u_Textures[int(round(v_TexIndex))], v_TexCoords);
    o_Color = texColor;
	o_EntityID = v_EntityID;
}