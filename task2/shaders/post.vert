#include "base.h"

out gl_PerVertex {   
	vec4 gl_Position;
};

layout(location = 0) out vec2 INOUT_TextureCoords;
layout(location = 1) out vec2 INOUT_TextureCoordsRendering;
layout(location = 2) out vec2 INOUT_TextureCoordsRenderingOld;
uniform vec2 RenderingScale;
uniform vec2 RenderingScaleOld;

void main() {
	float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    gl_Position = vec4(x, y, 0.9999, 1);
    INOUT_TextureCoords = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);
    INOUT_TextureCoordsRendering = INOUT_TextureCoords * RenderingScale;
    INOUT_TextureCoordsRenderingOld = INOUT_TextureCoords * RenderingScaleOld;
}