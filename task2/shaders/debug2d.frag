#include "base.h"

layout(location = 0) out vec4 OUT_Color;

layout(location = 0) in vec4 INOUT_Color;
layout(location = 1) in vec2 INOUT_TexCoord;

uniform sampler2D DebugTexture;

void main() {
    vec4 texColor = texture(DebugTexture, INOUT_TexCoord);
    OUT_Color = vec4(INOUT_Color.rgb, INOUT_Color.a * texColor.a);
}