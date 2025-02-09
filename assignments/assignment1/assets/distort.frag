#version 450

out vec4 FragColor;
in vec2 vs_texcoord;

uniform sampler2D texture0;
uniform vec3 radialDistortionParams;
uniform vec2 tangentialDistortionParams;

//radial and tangential lens distortion found:
//https://medium.com/@vlh2604/real-time-radial-and-tangential-lens-distortion-with-opengl-a55b7493e207

vec2 RadialDistortion(vec2 coord, float k1, float k2, float k3) 
{
    float r = length(coord);
    float distortionFactor = 1.0 + k1 * pow(r, 2) + k2 * pow(r, 4) + k3 * pow(r, 6);

    return distortionFactor * coord;
}

vec2 TangentialDistortion(vec2 coord, float p1, float p2) 
{
    float x = coord.x;
    float y = coord.y;

    float r2 = x * x + y * y;

    float dx = 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
    float dy = p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;

    return vec2(dx, dy);
}


void main() {
    vec2 coords = vs_texcoord * 2.0 - 1.0;

    vec2 radialDistortionCoords = RadialDistortion(coords, radialDistortionParams.x, radialDistortionParams.y, radialDistortionParams.z);
    vec2 tangentialDistortionCoords = TangentialDistortion(coords, tangentialDistortionParams.x, tangentialDistortionParams.y);

    vec2 distortedCoord = radialDistortionCoords + tangentialDistortionCoords;

    distortedCoord = (distortedCoord + 1.0) / 2.0;

    FragColor = texture(texture0, distortedCoord);
}