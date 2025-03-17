#version 450

//in attributes
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 3) in vec2 in_texcoord;

//out
out vec3 vs_normal;

//uniforms
uniform mat4 _CameraViewProj;
uniform mat4 _Model;

void main()
{
	gl_Position = _CameraViewProj * _Model * vec4(in_pos, 1.0);
}