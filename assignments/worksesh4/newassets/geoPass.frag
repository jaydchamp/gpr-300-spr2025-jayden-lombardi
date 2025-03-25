#version 450 core

//in attributes
in Surface{
	vec3 WorldPos; 
	vec2 TexCoord;
	vec3 WorldNormal;
}fs_in;

//out attributes
layout(location = 0) out vec3 vPos; //Worldspace position
layout(location = 1) out vec3 vNormal; //Worldspace normal 
layout(location = 2) out vec3 vAlbedo;

//uniforms
uniform sampler2D _MainTex;

void main()
{
	vPos = fs_in.WorldPos;
	vNormal = normalize(fs_in.WorldNormal);
	vAlbedo = texture(_MainTex,fs_in.TexCoord).rgb;
}
