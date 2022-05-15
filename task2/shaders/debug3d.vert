#include "base.h"

uniform mat4 ViewProjection;

layout(location = 0) in vec3 IN_Position;
layout(location = 1) in vec4 IN_Color;

out gl_PerVertex {   
	vec4 gl_Position;
};

layout(location = 0) out vec4 INOUT_Color;

void main() {
    vec4 worldPosition = vec4(IN_Position, 1);
    gl_Position = worldPosition * ViewProjection;
    INOUT_Color = IN_Color;
}
