#version 450

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D texture0; 

void main()
{
	//invert the color
	vec3 albedo = 1.0 - texture(texture0, vs_texcoord).rgb;
	FragColor = vec4(albedo, 1.0);
}