#version 450

out vec4 FragColor; 
in vec3 to_camera;
in vec2 vs_texcoords;

uniform vec3 waterColor;
uniform sampler2D texture0;
uniform float tilling;
uniform float time;
uniform float scrollSpeed;
uniform float sampler1Offset;
uniform float sampler2Offset;
uniform float b1;
uniform float b2;

const vec3 reflectColor = vec3(1.0, 0.0, 0.0);

void main()
{
	float fresnelEffect  = dot(normalize(to_camera), vec3(0.0, 1.0, 0.0));

	vec2 dir = normalize(vec2(1.0));
	vec2 uv = (vs_texcoords * tilling) + dir;// * time;

	//distort water effect:
	uv.y += 0.01 * (sin(uv.x * 3.5 + time * 0.35) + sin(uv.x * 4.8 + time * 1.05) + sin(uv.x * 7.3 + time * 0.45)) / 3.0;
    uv.x += 0.12 * (sin(uv.y * 4.0 + time * 0.5) + sin(uv.y * 6.8 + time * 0.75) + sin(uv.y * 11.3 + time * 0.2)) / 3.0;
    uv.y += 0.12 * (sin(uv.x * 4.2 + time * 0.64) + sin(uv.x * 6.3 + time * 1.65) + sin(uv.x * 8.2 + time * 0.45)) / 3.0;

	vec3 albedo = texture(texture0, uv).rgb;

	vec4 smp1 = texture(texture0, uv + vec2(sampler1Offset));
	vec4 smp2 = texture(texture0, uv + vec2(sampler2Offset));

	//vec3 color = mix(reflectColor, waterColor, fresnelEffect);
	vec3 color = waterColor + vec3(smp1.r * b1 - smp2.r * b2);

	FragColor = vec4(color, 1.0);
}
