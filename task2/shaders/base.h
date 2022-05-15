// Different material features that can be checked in the shader.
#define MATERIAL_FEATURE_ALBEDO_MAP 1
#define MATERIAL_FEATURE_METALLICROUGHNESS_MAP 2
#define MATERIAL_FEATURE_NORMAL_MAP 4
#define MATERIAL_FEATURE_OCCLUSION_MAP 8
#define MATERIAL_FEATURE_EMISSIVE_MAP 16
#define MATERIAL_FEATURE_OPACITY_TRANSPARENT 32
#define MATERIAL_FEATURE_OPACITY_CUTOUT 64

// Definition of supported light types.
#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIRECTIONAL 2

// Constants used for bvh construction.
#define BVH_MAX_TRIANGLES 10000000
#define BVH_MAX_NODES 5000000
#define BVH_USE_RANDOM 0
#define BVH_MAX_TRIANGLES_PER_NODE 1
#define BVH_MAX_CHILD_NODES 2

// Constants used for raytracing.
#define RENDERING_MAX_RECURSIONS 8
#define RENDERING_MAX_SAMPLES 1

// Constants used for temporal anti-aliasing.
#define RENDERING_TAA_SAMPLE_COUNT 16

// Constants used for performance measure.
#define RENDERING_FRAME_GPU_TIME_COUNT 3
#define RENDERING_FRAMETIMES_COUNT 128

#ifdef __cplusplus
    // Define some glsl types to use the corresponding c++ types so that we 
    // can define some structs below that are shared between glsl and c++.
    #define sampler2D uint64_t
    #define vec2 glm::vec2
    #define vec3 glm::vec3
    #define vec4 glm::vec4
    #define uint uint32_t
#else
    // Define some shared helper constants and functions for our shaders.
    const float EPSILON = 0.000001;
    const float NORMAL_EPSILON = 0.000001;

    const float RAND_PHI = 1.61803398874989484820459 * 00000.1; // Golden Ratio   
    const float RAND_PI  = 3.14159265358979323846264 * 00000.1; // PI
    const float RAND_SQ2 = 1.41421356237309504880169 * 10000.0; // Square Root of Two

    const float M_PI = 3.141592653589793;
    const float M_TWO_PI = 3.141592653589793 * 2.0;

    // From http://jcgt.org/published/0003/02/01/ and https://discourse.panda3d.org/t/glsl-octahedral-normal-packing/15233
    vec2 sign_not_zero(vec2 v) {
        return fma(step(vec2(0.0), v), vec2(2.0), vec2(-1.0));
    }

    // Packs a 3-component normal to 2 channels using octahedron normals
    vec2 encodeNormal(vec3 v) {
        v.xy /= dot(abs(v), vec3(1));
        return mix(v.xy, (1.0 - abs(v.yx)) * sign_not_zero(v.xy), step(v.z, 0.0));
    }

    // Unpacking from octahedron normals, input is the output from pack_normal_octahedron
    vec3 decodeNormal(vec2 packed_nrm) {
        vec3 v = vec3(packed_nrm.xy, 1.0 - abs(packed_nrm.x) - abs(packed_nrm.y));
        if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * sign_not_zero(v.xy);

        return normalize(v);
    }

    // This texture array contains all textures in our scene.
    uniform sampler2DArray MaterialTextures;
    vec4 SampleMaterialTexture(int textureIndex, vec2 texCoords) {
        if(textureIndex == -1) {
            return vec4(0, 0, 0, 0);
        }
        return texture(MaterialTextures, vec3(texCoords.x, texCoords.y, float(textureIndex)));
    }

    // Sometimes (e.g. when casting rays) the GPU can not automatically determine the mip level 
    // of a texture so we can use this function to just sample a specific mip level of a texture.
    vec4 SampleMaterialTextureLod(int textureIndex, vec2 texCoords, float mipLevel) {
        if(textureIndex == -1) {
            return vec4(0, 0, 0, 0);
        }
        return textureLod(MaterialTextures, vec3(texCoords.x, texCoords.y, float(textureIndex)), mipLevel);
    }

    // Some hardware (e.g. Mac) do not have unpackHalf2x16 functions so we define a custom one here.
    // Copyright 2002 The ANGLE Project Authors
    // https://chromium.googlesource.com/angle/angle/+/master/src/compiler/translator/BuiltInFunctionEmulatorGLSL.cpp
    #if !defined(GL_ARB_shading_language_packing)
        float f16tof32(uint val)
        {
            uint sign = (val & 0x8000u) << 16;
            int exponent = int((val & 0x7C00u) >> 10);
            uint mantissa = val & 0x03FFu;
            float f32 = 0.0;
            if(exponent == 0)
            {
                if (mantissa != 0u)
                {
                    const float scale = 1.0 / (1 << 24);
                    f32 = scale * mantissa;
                }
            }
            else if (exponent == 31)
            {
                return uintBitsToFloat(sign | 0x7F800000u | mantissa);
            }
            else
            {
                exponent -= 15;
                float scale;
                if(exponent < 0)
                {
                    // The negative unary operator is buggy on OSX.
                    // Work around this by using abs instead.
                    scale = 1.0 / (1 << abs(exponent));
                }
                else
                {
                    scale = 1 << exponent;
                }
                float decimal = 1.0 + float(mantissa) / float(1 << 10);
                f32 = scale * decimal;
            }

            if (sign != 0u)
            {
                f32 = -f32;
            }

            return f32;
        }
    #endif

    vec2 unpackHalf2x16_emu(uint u)
    {
        #if defined(GL_ARB_shading_language_packing)
            return unpackHalf2x16(u);
        #else
            uint y = (u >> 16);
            uint x = u & 0xFFFFu;
            return vec2(f16tof32(x), f16tof32(y));
        #endif
    }
#endif

// Shared material structure.
struct RendererMaterial {
    vec4 AlbedoFactor;

    vec3 EmissiveFactor;
    float RoughnessFactor;
    
    float MetalnessFactor;
    float OcclusionStrength; 
    float NormalScale;
    uint FeatureMask;

    float AlphaCutoff;
    int AlbedoMap;
    int MetallicRoughnessMap;
    int NormalMap;

    int OcclusionMap;
    int EmissiveMap;
};

bool HasFeature(RendererMaterial material, uint mask) {
    return (material.FeatureMask & mask) != 0;
}

// Shared light structure.
struct RendererLight {
    vec3 Position;
    float Range;
    
    vec3 Direction;
    uint Type;

    vec3 Color;
    float Intensity;

    float AngleScale;
    float AngleOffset;
    float Radius;
};

// All information needed to shade a surface point.
struct SurfacePoint {
    vec3 Position;
    vec3 Normal;
    vec4 Albedo;
    float Metalness;
    float Roughness;
};

#ifdef __cplusplus
    // Undef our defines from above so that we don't overwrite something by mistake that we need.
    #undef sampler2D
    #undef vec2 
    #undef vec3 
    #undef vec4 
    #undef uint
#else

    // Contains all lights from the scene.
    uniform usamplerBuffer LightBuffer;

    // Light count to loop over all lights in the scene for lighting.
    uniform int LightCount;

    // Decode the structure from the raw texture buffer.
    // On more modern hardware (OGL 4.2+) we could use shader storage buffers which 
    // have a nicer interface but are functionaly similar.
    RendererLight GetLight(int lightIndex) {
        int structOffset = lightIndex * 15;
        RendererLight light;
        light.Position.x = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Position.y = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Position.z = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Range      = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;

        light.Direction.x = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Direction.y = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Direction.z = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Type        = texelFetch(LightBuffer, structOffset).r; ++structOffset;

        light.Color.x   = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Color.y   = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Color.z   = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Intensity = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;

        light.AngleScale  = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.AngleOffset = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;
        light.Radius      = uintBitsToFloat(texelFetch(LightBuffer, structOffset).r); ++structOffset;

        return light;
    }

    // Contains all materials in the scene.
    uniform usamplerBuffer MaterialBuffer;

    // Decode material from the texture buffer.
    RendererMaterial GetMaterial(uint materialIndex) {
        int structOffset = int(materialIndex) * 18;
        RendererMaterial material;
        material.AlbedoFactor.x = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.AlbedoFactor.y = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.AlbedoFactor.z = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.AlbedoFactor.w = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;

        material.EmissiveFactor.x = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.EmissiveFactor.y = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.EmissiveFactor.z = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.RoughnessFactor  = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;

        material.MetalnessFactor   = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.OcclusionStrength = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.NormalScale       = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.FeatureMask       = texelFetch(MaterialBuffer, structOffset).r; ++structOffset;

        material.AlphaCutoff          = uintBitsToFloat(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.AlbedoMap            = int(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.MetallicRoughnessMap = int(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.NormalMap            = int(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;

        material.OcclusionMap = int(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;
        material.EmissiveMap  = int(texelFetch(MaterialBuffer, structOffset).r); ++structOffset;

        return material;
    }
#endif
