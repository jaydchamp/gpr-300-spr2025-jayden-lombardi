#version 450 core

//out attributes
out vec4 FragColor;

//in attributes
in vec2 vs_texcoord;

//uniforms
uniform layout(binding = 0) sampler2D _Positions;
uniform layout(binding = 1) sampler2D _Normals;
uniform layout(binding = 2) sampler2D _Albedo;

void main()
{
	vec3 worldPos = texture(_Positions, vs_texcoord).xyz;
	vec3 normal = texture(_Normals, vs_texcoord).xyz;
	vec3 albedo = texture(_Albedo, vs_texcoord).xyz;

	//vec3 lightColor = calculateLighting(normal,worldPos,albedo);

	FragColor = vec4(albedo/* * lightColor*/,1.0);
}
