#version 450

//out attributes
out vec4 FragColor;

//in
in vec2 vs_texcoord;

//uniforms
uniform sampler2D _Albedo;
uniform sampler2D _LightingTexture;
uniform sampler2D _Lights;

void main() 
{
   vec3 objectColor = texture(_Albedo, vs_texcoord).rgb;
   vec3 lightingColor = texture(_LightingTexture, vs_texcoord).rgb;
   vec4 lights = texture(_Lights, vs_texcoord).rgba;
   
   vec3 finalLighting = lightingColor * objectColor;
   finalLighting = mix(finalLighting, lights.rgb, lights.a);

   FragColor = vec4(finalLighting, 1.0);
}