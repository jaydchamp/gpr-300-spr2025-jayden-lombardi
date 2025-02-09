#version 450

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D texture0; 
uniform float _exposure;

const float gamma = 2.2;

void main()
{
	vec3 hdr = texture(texture0, vs_texcoord).rgb;

	vec3 mapped = vec3(1.0) - exp(-hdr * _exposure);	//tone mapping
	mapped = pow(mapped, vec3(1.0 / gamma));			//gamma correction

	FragColor = vec4(mapped, 1.0);
}