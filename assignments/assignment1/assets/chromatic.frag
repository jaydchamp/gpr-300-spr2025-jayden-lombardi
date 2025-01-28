#version 450

//what is coming out of the shader
out vec4 FragColor;
in vec2 vs_texcoord;

uniform sampler2D texture0; 
const vec3 offset = vec3(0.009, 0.006, -0.006);
const vec2 direction = vec2(1.0, 0.0);

void main()
{
	FragColor.r = texture(texture0, vs_texcoord + (direction * vec2(offset.r))).r;
	FragColor.g = texture(texture0, vs_texcoord + (direction * vec2(offset.r))).g;
	FragColor.b = texture(texture0, vs_texcoord + (direction * vec2(offset.r))).b;
}