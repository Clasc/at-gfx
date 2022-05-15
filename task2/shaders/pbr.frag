#include "base.h"
#include "random.h"
#include "raytrace.h"
#include "lighting.h"

layout(location = 0) out vec4 OUT_Color;
layout(location = 0) in vec2 INOUT_TextureCoords;
layout(location = 1) in vec2 INOUT_GBufferTextureCoords;


void main() {
    vec2 texCoord = INOUT_GBufferTextureCoords; 

	// Fetch and compute surface aspects from gbuffer.
	float depthFromBuffer = 2.0 * textureLod(GBufferDepth, texCoord, 0).r - 1.0;
	vec2 normalXY = textureLod(GBufferNormal, texCoord, 0).rg;
	vec3 normal = decodeNormal(normalXY);
	vec4 albedoTransparency = textureLod(GBufferAlbedoTransparency, texCoord, 0);
	vec2 metalnessRoughness = textureLod(GBufferMetallnessRoughness, texCoord, 0).rg;

	vec4 projectedPosition = vec4(INOUT_TextureCoords * 2.0 - 1.0, depthFromBuffer, 1.0);
    vec4 worldPosBeforeW = projectedPosition * CameraInvViewProjection;
	vec3 worldPosition = worldPosBeforeW.xyz / worldPosBeforeW.w;

    SurfacePoint point;
    point.Position = worldPosition;
    point.Normal = normal;
    point.Albedo = albedoTransparency;
    point.Metalness = metalnessRoughness.r;
    point.Roughness = metalnessRoughness.g;

    vec3 viewVec = normalize(CameraPosition - worldPosition); 
    ShadingResult result = ShadePoint(point, viewVec);
    OUT_Color = vec4(result.Diffuse + result.Specular, 1.0);
}