#version 450

//out attributes
layout (location = 4) out vec4 FragColor4;

//in
in vec3 vs_normal;

//uniforms
uniform vec3 _Color;

void main() 
{
	FragColor4 = vec4(_Color, 1.0);
}