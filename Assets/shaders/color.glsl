// Basic Texure Shader

#type vertex
#version 330 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoords;


uniform mat4 u_ViewProjection;
uniform mat4 u_Model;

void main()
{
    gl_Position =  u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}

// ===================================================================================================================
#type fragment
#version 330 core
layout (location = 0) out vec4 FragColor;

const float PI = 3.14159265359;

void main()
{
    FragColor = vec4(1.0f);
}