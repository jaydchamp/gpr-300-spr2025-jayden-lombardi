#version 450

//in attributes
layout(location = 0) in vec3 vPos;	  
layout(location = 1) in vec3 vNormal; 
layout(location = 2) in vec2 vAlbedo; 

//out attributes
out Surface{
	vec3 WorldPos;
	vec2 TexCoord;
	vec3 WorldNormal;
}vs_out;

//uniforms
uniform mat4 _Model; 

void main()
{
	vs_out.WorldPos = vec3(_Model * vec4(vPos, 1.0));
	vs_out.WorldNormal = transpose(inverse(mat3(_Model))) * vNormal;

	vs_out.TexCoord = vAlbedo;
	gl_Position = _Model * vec4(vPos, 1.0);
}
