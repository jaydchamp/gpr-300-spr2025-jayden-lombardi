#version 450

//what is coming out of the shader
out vec4 FragColor;
in vec2 vs_texcoord;

uniform sampler2D texture0; 

void main()
{
	vec3 albedo = texture(1.0 - texture0, vs_texcoord).rgb;
	FragColor = vec4(albedo, 1.0);
}