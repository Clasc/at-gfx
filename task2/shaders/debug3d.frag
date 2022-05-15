#include "base.h"

layout(location = 0) out vec4 OUT_Color;

layout(location = 0) in vec4 INOUT_Color;

void main() {
	OUT_Color = INOUT_Color;
}