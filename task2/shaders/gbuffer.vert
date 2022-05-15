#include "base.h"

uniform mat4 ViewProjection;
uniform mat4 ViewProjectionOld;
uniform mat4 World;
uniform mat4 WorldInvTrans;

layout(location = 0) in vec3 IN_Position;
layout(location = 1) in vec3 IN_Normal;
layout(location = 2) in vec2 IN_TexCoord;

out gl_PerVertex {   
	vec4 gl_Position;
};

layout(location = 0) out vec3 INOUT_WorldPos;
layout(location = 1) out vec3 INOUT_Normal;
layout(location = 2) out vec2 INOUT_TexCoord;
layout(location = 3) out vec4 INOUT_ViewOld;
layout(location = 4) out vec4 INOUT_ViewNew;

void main() {
    vec4 worldPosition = World * vec4(IN_Position, 1);
    INOUT_WorldPos = worldPosition.xyz;
    INOUT_Normal = (WorldInvTrans * vec4(IN_Normal, 0)).xyz;
    INOUT_TexCoord = IN_TexCoord;
    INOUT_ViewOld = worldPosition * ViewProjectionOld;
    INOUT_ViewNew = worldPosition * ViewProjection;

    gl_Position = INOUT_ViewNew;
}
