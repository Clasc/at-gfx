#include "base.h"

layout(location = 0) out vec4 OUT_Normal;
layout(location = 1) out vec4 OUT_AlbedoTransparency;
layout(location = 2) out vec4 OUT_MetalnessRoughness;
layout(location = 3) out vec4 OUT_Motion;

layout(location = 0) in vec3 INOUT_WorldPos;
layout(location = 1) in vec3 INOUT_Normal;
layout(location = 2) in vec2 INOUT_TexCoord;
layout(location = 3) in vec4 INOUT_ViewOld;
layout(location = 4) in vec4 INOUT_ViewNew;

uniform int MaterialIndex;
uniform vec2 Jitter;
uniform vec2 JitterOld;


void main() {
	RendererMaterial material = GetMaterial(MaterialIndex);
    vec4 albedo = material.AlbedoFactor;
    if(HasFeature(material, MATERIAL_FEATURE_ALBEDO_MAP)) {
    	albedo *= SampleMaterialTexture(material.AlbedoMap, INOUT_TexCoord);	
    }

    if(HasFeature(material, MATERIAL_FEATURE_OPACITY_CUTOUT)) {
    	if (albedo.a < material.AlphaCutoff) {
		    discard;
		}
    }

    float metallic = material.MetalnessFactor;
    float roughness = material.RoughnessFactor;
	if(HasFeature(material, MATERIAL_FEATURE_METALLICROUGHNESS_MAP)) {
    	vec2 metallicRoughness = SampleMaterialTexture(material.MetallicRoughnessMap, INOUT_TexCoord).rg;
    	metallic *= metallicRoughness.r;
    	roughness *= metallicRoughness.g;	
    }


	OUT_Normal = vec4(encodeNormal(INOUT_Normal), 0.0, 1.0);
	OUT_AlbedoTransparency = albedo;
	OUT_MetalnessRoughness = vec4(metallic, roughness, 0, 1.0);
    OUT_Motion = vec4((((INOUT_ViewOld.xy / INOUT_ViewOld.w) - JitterOld) - ((INOUT_ViewNew.xy / INOUT_ViewNew.w) - Jitter)) * vec2(0.5, 0.5), 0, 0);
}