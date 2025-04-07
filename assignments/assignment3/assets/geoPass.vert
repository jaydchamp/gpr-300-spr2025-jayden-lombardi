#version 450

//in attributes
layout(location = 0) in vec3 vPos;	  
layout(location = 1) in vec3 vNormal; 
layout(location = 2) in vec2 vTextureCoord; 

//out attributes
out Surface{
	vec3 WorldPos;
	vec2 TexCoord;
	vec3 WorldNormal;
}vs_out;

//uniforms
uniform mat4 Model; 
uniform mat4 ViewProj; 

void main()
{
	vs_out.WorldPos = vec3(Model * vec4(vPos, 1.0));
	vs_out.WorldNormal = transpose(inverse(mat3(Model))) * vNormal;

	vs_out.TexCoord = vTextureCoord;
	gl_Position = ViewProj * Model * vec4(vPos, 1.0);
}
