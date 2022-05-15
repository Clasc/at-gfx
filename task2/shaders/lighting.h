uniform sampler2D GBufferDepth;
uniform sampler2D GBufferNormal;
uniform sampler2D GBufferAlbedoTransparency;
uniform sampler2D GBufferMetallnessRoughness;
uniform sampler2D GBufferMotion;

uniform uint FrameCount;

uniform vec3 CameraPosition;
uniform mat4 CameraInvViewProjection;

const vec3 dielectricSpecular = vec3(0.04, 0.04, 0.04);
const vec3 black = vec3(0, 0, 0);
const vec3 AmbientLight = vec3(0.4, 0.4, 0.25) * 10.0;
const float SurfaceOffset = 0.01;

struct ShadingResult {
	vec3 Diffuse;
	vec3 Specular;
};


// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 diffuse(vec3 diffuseColor)
{
	return diffuseColor / M_PI;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(vec3 reflectance0, vec3 reflectance90, float VdotH)
{
	return reflectance0 + (reflectance90 - reflectance0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(float NdotL, float NdotV, float alphaRoughness)
{
	float roughnessSq = alphaRoughness * alphaRoughness;
	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(roughnessSq + (1.0 - roughnessSq) * (NdotL * NdotL)));
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(roughnessSq + (1.0 - roughnessSq) * (NdotV * NdotV)));
	return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(float NdotH, float alphaRoughness)
{
	float roughnessSq = alphaRoughness * alphaRoughness;
	float f = (NdotH * roughnessSq - NdotH) * NdotH + 1.0;
	return roughnessSq / (M_PI * f * f);
}

float GetLightAttenuationAndLightVec(vec3 position, inout RendererLight light, out vec3 lightVec) {
    float attenuation = 0.0;
    if(light.Type == LIGHT_TYPE_POINT || light.Type == LIGHT_TYPE_SPOT) {
	   	lightVec = light.Position - position;
    	float distance = length(lightVec);
     	lightVec = lightVec / distance;
	   	if(distance < light.Range) {
	     	float a = (distance / light.Range);
	     	attenuation = max(min( 1.0 - a * a * a * a, 1.0 ), 0.0 ) / (distance * distance);

	     	if(light.Type == LIGHT_TYPE_SPOT) {
	     		float cd = dot(-light.Direction, lightVec);
				float angularAttenuation = clamp(cd * light.AngleScale + light.AngleOffset, 0.0, 1.0);
				attenuation *= angularAttenuation * angularAttenuation;
	     	}
	   	}
    } else { // LIGHT_TYPE_DIRECTIONAL
     	lightVec = -light.Direction;
     	light.Position = position + lightVec * 40.0;
 		attenuation = 1.0;
    }
    return attenuation;
}

ShadingResult ShadePointSimple(SurfacePoint point) 
{
    vec3 surfacePos = point.Position + point.Normal * SurfaceOffset;
    vec3 diffuseColor = mix(point.Albedo.rgb * (1.0 - dielectricSpecular.r), black, point.Metalness);

    ShadingResult result;
    result.Diffuse = vec3(0, 0, 0);
    result.Specular = vec3(0, 0, 0);
    for(int i = 0; i < LightCount; ++i) {
    	RendererLight light = GetLight(i);
    	vec3 lightVec;
    	float attenuation = GetLightAttenuationAndLightVec(point.Position, light, lightVec);
    	if(attenuation > 0.0) {
		    if(!CastVisRay(light.Position, surfacePos)) {
				float NdotL = clamp(dot(point.Normal, lightVec), 0.001, 1.0);
				result.Diffuse += light.Color * diffuseColor * (NdotL * attenuation * light.Intensity / M_PI);
	 		}
 		}
    }

    // Add some ambient.
	result.Diffuse += diffuseColor * AmbientLight;

    return result;
}

ShadingResult ShadePoint(SurfacePoint point, vec3 viewDirection) 
{
    vec3 surfacePos = point.Position + point.Normal * SurfaceOffset;
    float alphaRoughness = point.Roughness * point.Roughness;

    vec3 diffuseColor = mix(point.Albedo.rgb * (1.0 - dielectricSpecular.r), black, point.Metalness);
    vec3 specularColor = mix(dielectricSpecular, point.Albedo.rgb, point.Metalness);

	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	vec3 specularEnvironmentR0 = specularColor.rgb;
	vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;

	float seed = surfacePos.x + surfacePos.y * 3.43121412313 + surfacePos.z * 5.1231491 + float(FrameCount) * 0.177421;

    ShadingResult result;
    result.Diffuse = vec3(0, 0, 0);
    result.Specular = vec3(0, 0, 0);

    for(int i = 0; i < LightCount; ++i) {
    	RendererLight light = GetLight(i);
    	vec3 lightVec;
    	float attenuation = GetLightAttenuationAndLightVec(point.Position, light, lightVec);
    	if(attenuation > 0.0) {
		    if(!CastVisRay(light.Position, surfacePos)) {
				float NdotL = clamp(dot(point.Normal, lightVec), 0.001, 1.0);
				result.Diffuse += light.Color * diffuseColor * (NdotL * attenuation * light.Intensity / M_PI);
	 		}
 		}

	    vec3 halfVec = normalize(lightVec + viewDirection);

		float NdotL = clamp(dot(point.Normal, lightVec), 0.001, 1.0);
		float NdotV = clamp(abs(dot(point.Normal, viewDirection)), 0.001, 1.0);
		float NdotH = clamp(dot(point.Normal, halfVec), 0.0, 1.0);
		float LdotH = clamp(dot(lightVec, halfVec), 0.0, 1.0);
		float VdotH = clamp(dot(viewDirection, halfVec), 0.0, 1.0);

		// Calculate the shading terms for the microfacet specular shading model
		vec3 F = specularReflection(specularEnvironmentR0, specularEnvironmentR90, VdotH);
		float G = geometricOcclusion(NdotL, NdotV, alphaRoughness);
		float D = microfacetDistribution(NdotH, alphaRoughness);

		// Calculation of analytical lighting contribution
		// Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
		vec3 lighting = (NdotL * attenuation * light.Intensity) * light.Color;
		result.Diffuse += (diffuseColor / M_PI) * (1.0 - F) * lighting;
		result.Specular += (lighting * F * G) * D / (4.0 * NdotL * NdotV);
    }

    // Add some reflection, this is not 100% pbr.
    SurfacePoint nextPoint = point;
    vec3 lastViewDir = viewDirection;
    for(int i = 0; i < RENDERING_MAX_RECURSIONS; ++i) {
    	seed = mod( seed * 1.1234567893490423, 13. );
	    if(nextPoint.Roughness < 0.5 || nextPoint.Metalness > 0.5) {
			vec3 reflectionVec = normalize(reflect(-lastViewDir, nextPoint.Normal));
			vec3 glossyReflectionVec = normalize(mix(reflectionVec, randomHemisphereDirection(reflectionVec, seed), nextPoint.Roughness));
			if(CastSurfaceRay(surfacePos, surfacePos + glossyReflectionVec * 40.0, nextPoint)) {
				lastViewDir = -glossyReflectionVec;
				surfacePos = nextPoint.Position + nextPoint.Normal * SurfaceOffset;
				ShadingResult reflection = ShadePointSimple(nextPoint); 
				result.Specular += reflection.Diffuse + reflection.Specular;
			} else {
				break;
			}
		} 
    }

 	// Add refraction.
 	surfacePos = point.Position - point.Normal * 0.002;
    nextPoint = point;
    lastViewDir = viewDirection;
    for(int i = 0; i < 2; ++i) {
    	if(nextPoint.Albedo.a > 0.0 && nextPoint.Albedo.a < 1.0) {
    		vec3 refractionVec = normalize(refract(-lastViewDir, nextPoint.Normal, 0.9f));
			if(CastSurfaceRay(surfacePos, surfacePos + refractionVec * 40.0, nextPoint)) {
				lastViewDir = -refractionVec;
				surfacePos = nextPoint.Position - nextPoint.Normal * 0.002;
				ShadingResult refraction = ShadePointSimple(nextPoint); 
				result.Specular += refraction.Diffuse + refraction.Specular;
			} else {
				break;
			}
    	}
    }

    // Add some ambient.
	result.Diffuse += diffuseColor * AmbientLight;

    return result;
}
