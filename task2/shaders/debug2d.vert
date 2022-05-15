#include "base.h"

layout(location = 0) in vec3 IN_Position;
layout(location = 1) in vec4 IN_Color;
layout(location = 2) in vec2 IN_Texcoord;

out gl_PerVertex {   
	vec4 gl_Position;
};

layout(location = 0) out vec4 INOUT_Color;
layout(location = 1) out vec2 INOUT_Texcoord;

void main() {
    gl_Position = vec4(IN_Position, 1);
    INOUT_Color = IN_Color;
    INOUT_Texcoord = IN_Texcoord;
}
