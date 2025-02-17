#version 450

out vec4 FragColor; 
in vec3 to_camera;
in vec2 vs_texcoords;

uniform vec3 waterColor;
uniform sampler2D wave_tex;
uniform sampler2D wave_spec;
uniform sampler2D wave_warp;

uniform float warp_scale;
uniform float warp_strength;
uniform float spec_scale;

uniform float tilling;
uniform float time;
uniform float scrollSpeed;
uniform float sampler1Offset;
uniform float sampler2Offset;
uniform float b1;
uniform float b2;

uniform float brightness_lower_cutoff;
uniform float brightness_upper_cutoff;

const vec3 reflectColor = vec3(1.0, 0.0, 0.0);

void main()
{
	vec2 warp_uv = vs_texcoords * warp_scale;
	vec2 warp_scroll = vec2(0.5, 0.5) * time;
	vec2 warp = texture(wave_warp, warp_uv + warp_scroll).xy * warp_strength;

	warp = (warp * 2.0 - 1.0);

	vec2 albedo_uv = vs_texcoords * warp_scale;
	vec2 albedo_scroll = vec2(-0.5, 0.5) * time;
	vec4 albedo = texture(wave_tex, albedo_uv + warp + albedo_scroll);

	//specular
	vec2 spec_uv = vs_texcoords * spec_scale;
	vec3 smp1 = texture(wave_spec, spec_uv + vec2(1.0, 0.0) * time).rgb;
	vec3 smp2 = texture(wave_spec, spec_uv + vec2(1.0, 1.0) * time).rgb;
	vec3 spec = smp1 + smp2;

	float brightness = dot(spec, vec3(0.299, 0.587, 0.114));
	float fresnelEffect  = dot(normalize(to_camera), vec3(0.0, 1.0, 0.0));

//	if(brightness <= brightness_lower_cutoff || brightness > brightness_upper_cutoff)
//	{
		//not within brightness range
		//this is what sunshine does
//		discard;
//	}
//	else
//	{
		//use it!
//	}

	vec3 finalColor = mix(spec, waterColor + vec3(albedo.a), fresnelEffect);
	FragColor = vec4(finalColor, 1.0);
}
