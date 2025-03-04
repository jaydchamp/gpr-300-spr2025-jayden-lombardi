#version 450

//Vertex attributes
layout(location = 0) in vec3 in_position;	  
layout(location = 1) in vec3 in_normal; 
layout(location = 2) in vec2 in_texcoord; 

//uniform
uniform mat4 _Model;
uniform mat4 _ViewProj;

//out
out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoord;

void main()
{
	gl_Position = vec4(in_position.xy, 0.0, 1.0);
	vs_texcoord = in_texcoord;
}