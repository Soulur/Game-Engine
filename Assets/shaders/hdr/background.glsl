// Basic Texure Shader

#type vertex
#version 330 core
layout (location = 0) in vec3 a_Position;

uniform mat4 projection;
uniform mat4 view;

out vec3 WorldPos;

void main()
{
    WorldPos = a_Position;

	mat4 rotView = mat4(mat3(view));
	vec4 clipPos = projection * rotView * vec4(WorldPos, 1.0);

	gl_Position = clipPos.xyww;
}

// ===================================================================================================================
#type fragment
#version 330 core
layout (location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

in vec3 WorldPos;

uniform samplerCube environmentMap;

void main()
{		
    vec3 envColor = texture(environmentMap, WorldPos).rgb;
    
    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
    
    o_Color = vec4(envColor, 1.0);
    o_EntityID = -1;
}
