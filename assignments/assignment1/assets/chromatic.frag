#version 450

//what is coming out of the shader
out vec4 FragColor;
in vec2 vs_texcoord;

uniform sampler2D texture0; 

//color shift amounts
const vec3 offset = vec3(0.009, 0.006, -0.006);
const vec2 direction = vec2(1.0, 0.0);

void main()
{
	float r = texture(texture0, vs_texcoord + (direction * vec2(offset.r))).r;
	float g = texture(texture0, vs_texcoord + (direction * vec2(offset.g))).g;
	float b = texture(texture0, vs_texcoord + (direction * vec2(offset.b))).b;

	FragColor = vec4(r, g, b, 1.0);
}