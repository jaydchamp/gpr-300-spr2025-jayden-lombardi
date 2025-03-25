#version 450

//out attributes
layout (location = 0) out vec4 FragColor0;
layout (location = 1) out vec4 FragColor1;
layout (location = 2) out vec4 FragColor2;

//in 
in vec3 vs_world_pos;
in vec3 vs_world_normal;
in vec2 vs_texcoord;

//uniforms
uniform sampler2D _MainTexture;

void main()
{
	vec3 normal = normalize(vs_world_normal);

	vec3 objectColor0 = texture(_MainTexture, vs_texcoord).rgb;
	vec3 objectColor1 = vs_world_pos.xyz;
	vec3 objectColor2 = vec3(normal.xyz * 0.5 + 0.5);

	FragColor0 = vec4(objectColor0, 1.0);
	FragColor1 = vec4(objectColor1, 1.0);
	FragColor2 = vec4(objectColor2, 1.0);
}