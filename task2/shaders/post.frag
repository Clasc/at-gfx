#include "base.h"

layout(location = 0) out vec4 OUT_Color;

layout(location = 0) in vec2 INOUT_TextureCoords;
layout(location = 1) in vec2 INOUT_TextureCoordsRendering;
layout(location = 2) in vec2 INOUT_TextureCoordsRenderingOld;

uniform sampler2D IntermediateBuffer;
uniform float CameraExposure;

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;

float W = 11.2;



float Uncharted2Tonemap(float x) {
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 Uncharted2Tonemap(vec3 x) {
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}


void main() {
	float Exposure = exp2(CameraExposure);
	vec3 hdr = textureLod(IntermediateBuffer, INOUT_TextureCoordsRendering, 0).rgb * Exposure;

    float whiteScale = 1.0 / Uncharted2Tonemap(W);
    vec3 ldr = Uncharted2Tonemap(hdr);
    ldr = ldr * whiteScale;

    OUT_Color = vec4(ldr, 1.0);
}