#version 450 core

//out attributes
out vec4 FragColor; 

//in attributes
in vec2 UV;

//structs
struct Material 
{
    float ambientK; // Ambient
    float diffuseK; // Diffuse
    float specularK; // Specular
    float shininess;
};
struct PointLight 
{
    vec3 pos;
    float radius;
    vec4 color;
};

uniform layout(binding = 0) sampler2D Positions;
uniform layout(binding = 1) sampler2D Normals;
uniform layout(binding = 2) sampler2D Albedo;
uniform layout(binding = 3) sampler2D ShadowMap;

//uniforms
uniform vec3 LightDir;
uniform vec3 LightColor;
uniform float LightIntensity;
uniform vec3 CameraPos;

uniform mat4 LightSpaceMatrix;
uniform float MinBias;
uniform float MaxBias;
uniform float ShadowSof;

uniform Material material;

#define MAX_POINT_LIGHTS 64
uniform PointLight PointLights[MAX_POINT_LIGHTS];
uniform int PointLightCount;

float attenuateLinear(float distance, float radius) 
{
    return clamp((radius - distance) / radius, 0.0, 1.0);
}

float attenuateExponential(float distance, float radius) 
{
    float i = clamp(1.0 - pow(distance / radius, 4.0), 0.0, 1.0);
    return i * i;
}

float calculateShadow(vec3 worldPos, vec3 normal) 
{
    float shadow = 0.0;
    // Transform to light space
    vec4 posLightSpace = LightSpaceMatrix * vec4(worldPos, 1.0);
    
    // Perspective divide
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;    

    if (currentDepth > 1.0) {
        return 0.0; // Not in shadow
    }

    float bias = max(MinBias * (1.0 - dot(normal, -LightDir)), MaxBias);
    
    vec2 texelSize = 1.0 / textureSize(ShadowMap, 0);

    
    for(int x = -1; x <= 1; ++x) 
    {
        for(int y = -1; y <= 1; ++y) 
        {
            float closestDepth = texture(ShadowMap, projCoords.xy + vec2(x, y) * texelSize * ShadowSof).r;
            shadow += ((currentDepth - bias) > closestDepth) ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

vec3 calculateDirectionalLight(vec3 normal, vec3 worldPos, vec3 albedo) 
{
    // Calculate view direction
    vec3 viewDir = normalize(CameraPos - worldPos);
    vec3 lightDir = normalize(-LightDir);
    
    //ambient
    vec3 ambient = material.ambientK * albedo;
    
    //diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = material.diffuseK * diff * albedo * LightColor;
    
    //specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = material.specularK * spec * LightColor;
    
    float shadow = calculateShadow(worldPos, normal);
    return ambient + (1.0 - shadow) * (diffuse + specular) * LightIntensity;
}

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 worldPos, vec3 albedo) 
{
    // Calculate view direction
    vec3 viewDir = normalize(CameraPos - worldPos);
    
    // Calculate light direction
    vec3 lightDir = normalize(light.pos - worldPos);
    
    //diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = material.diffuseK * diff * albedo * light.color.rgb;
    
    //specular
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = material.specularK * spec * light.color.rgb;
    
    float distance = length(light.pos - worldPos);
    float attenuation = attenuateExponential(distance, light.radius);
   
    return (diffuse + specular) * light.color.rgb * light.color.a * attenuation * 2.0;
}

void main() 
{
    vec3 normal = normalize(texture(Normals, UV).xyz);
    vec3 worldPos = texture(Positions, UV).xyz;
    vec3 albedo = texture(Albedo, UV).xyz;
    
    vec3 totalLight = calculateDirectionalLight(normal, worldPos, albedo);

    vec3 pointLightContribution = vec3(0.0);
    for(int i = 0; i < PointLightCount; i++) 
    {
        vec3 lightContrib = calculatePointLight(PointLights[i], normal, worldPos, albedo);
        pointLightContribution += lightContrib;
        totalLight += lightContrib;
    }

    FragColor = vec4(totalLight, 1.0);
}