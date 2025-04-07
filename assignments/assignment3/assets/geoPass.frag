#version 450 core

//out attributes
layout(location = 0) out vec3 vPos; //Worldspace position
layout(location = 1) out vec3 vNormal; //Worldspace normal 
layout(location = 2) out vec3 vAlbedo;

//in attributes
in Surface{
	vec3 WorldPos; 
	vec2 TexCoord;
	vec3 WorldNormal;
}fs_in;


//uniforms
uniform sampler2D MainTex;

void main()
{
	vPos = fs_in.WorldPos;
	vAlbedo = texture(MainTex,fs_in.TexCoord).rgb;
	vNormal = normalize(fs_in.WorldNormal);
}
