#define DEBUG_DRAW_CIRCLE_STEPS 32
#define DEBUG_VERTEX_COUNT 12800000
#define DEBUG_COMMAND_COUNT 12800000

namespace Debug
{
    struct DebugVertex
    {
        glm::vec3 Position;
        uint32_t Color;
        glm::vec2 UV;
    };

    enum CommandTypes {
        CommandTypesDrawPoints,
        CommandTypesDrawLines,
        CommandTypesDrawTriangles,
        CommandTypesDrawScreenSpaceTriangles,
        CommandTypesSetTexture,
        CommandTypesSetBlending,
        CommandTypesSetScissor,
        CommandTypesCount
    };

    struct RendererCommand {
        CommandTypes CommandType;
        union {
            struct {
                int32_t VertexStart;
                int32_t VertexCount;
            } DrawData;
            struct 
            {
                int32_t TextureTarget;
                GLuint TextureHandle;
            } TextureData;   
            struct 
            {
                bool UseAlphaBlending;
            } BlendingData;   
            struct 
            {
                Shader* Pipeline;
            } ShaderData;
            struct 
            {
                uint16_t ScissorX;
                uint16_t ScissorY;
                uint16_t ScissorWidth;
                uint16_t ScissorHeight;
            } ScissorData;
            uint64_t CompareValue;    
        };
    };

    struct DebugRenderer
    {
        DebugVertex Vertices[DEBUG_VERTEX_COUNT];
        int32_t VertexCount; 
        GLuint VertexBuffer;
        GLuint VAO;
        bool TooManyVertices;

        RendererCommand Commands[DEBUG_COMMAND_COUNT];
        int32_t CommandCount;
        bool TooManyCommands;
        RendererCommand* LastCommand;

        GLuint CameraUniformBuffer;

        Shader* Shader3D;
        Shader* Shader2D;
    };

    static DebugRenderer DebugRendererInstance;

    void
    InitializeRenderer() {
        // Screenspace Triangles
        glGenVertexArrays(1, &DebugRendererInstance.VAO);
        glBindVertexArray(DebugRendererInstance.VAO);

        glGenBuffers(1, &DebugRendererInstance.VertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, DebugRendererInstance.VertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(DebugRendererInstance.Vertices), 0, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (const void *)offsetof(DebugVertex, Position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DebugVertex), (const void *)offsetof(DebugVertex, Color));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (const void *)offsetof(DebugVertex, UV));

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        DebugRendererInstance.Shader3D = new Shader("Debug 3D", "../../shaders/debug3d.vert", "../../shaders/debug3d.frag");
        DebugRendererInstance.Shader2D = new Shader("Debug 2D", "../../shaders/debug2d.vert", "../../shaders/debug2d.frag");
    }

    void
    PushVertex(glm::vec3 position, uint32_t color, glm::vec2 uv) {
        if(DebugRendererInstance.TooManyVertices) {
            return;
        }

        DebugVertex* vert = &DebugRendererInstance.Vertices[DebugRendererInstance.VertexCount];
        vert->Position = position;
        vert->Color = color;
        vert->UV = uv;
        ++DebugRendererInstance.VertexCount;

        DebugRendererInstance.TooManyVertices = (DebugRendererInstance.VertexCount == ArrayCount(DebugRendererInstance.Vertices));
    }

    void
    PushCommand(RendererCommand command) {
        bool commandConsumed = false;
        if(DebugRendererInstance.LastCommand) {
            if(DebugRendererInstance.LastCommand->CommandType == command.CommandType) 
            {
                if(DebugRendererInstance.LastCommand->CommandType <= CommandTypesDrawScreenSpaceTriangles) {
                    DebugRendererInstance.LastCommand->DrawData.VertexCount += command.DrawData.VertexCount;
                    commandConsumed = true;
                } else if(DebugRendererInstance.LastCommand->CompareValue == command.CompareValue) {
                    commandConsumed = true;
                }
            }
        }

        if(!commandConsumed) {
            if(DebugRendererInstance.TooManyCommands) {
                return;
            }

            DebugRendererInstance.Commands[DebugRendererInstance.CommandCount] = command;
            DebugRendererInstance.LastCommand = DebugRendererInstance.Commands + DebugRendererInstance.CommandCount;
            
            ++DebugRendererInstance.CommandCount;
            DebugRendererInstance.TooManyCommands = (DebugRendererInstance.CommandCount == ArrayCount(DebugRendererInstance.Commands));
        }
    }

    void 
    SetScissorRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
        RendererCommand command;
        command.CommandType = CommandTypesSetScissor;
        command.ScissorData.ScissorX = x;
        command.ScissorData.ScissorY = y;
        command.ScissorData.ScissorWidth = width;
        command.ScissorData.ScissorHeight = height;
        PushCommand(command);
    }

    void 
    DrawPoint(glm::vec3 point, uint32_t color) {
        RendererCommand command;
        command.CommandType = CommandTypesDrawPoints;
        command.DrawData.VertexStart = DebugRendererInstance.VertexCount;
        command.DrawData.VertexCount = 1;
        PushCommand(command);

        PushVertex(point, color, glm::vec2());
    }

    void 
    DrawLine(glm::vec3 start, glm::vec3 end, uint32_t color) {
        RendererCommand command;
        command.CommandType = CommandTypesDrawLines;
        command.DrawData.VertexStart = DebugRendererInstance.VertexCount;
        command.DrawData.VertexCount = 2;
        PushCommand(command);

        PushVertex(start, color, glm::vec2());
        PushVertex(end, color, glm::vec2());
    }

    void 
    DrawTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 c, uint32_t color) {
        DrawLine(a, b, color);
        DrawLine(b, c, color);
        DrawLine(c, a, color);
    }


    void 
    DrawTriangleNormal(glm::vec3 a, glm::vec3 b, glm::vec3 c, uint32_t color, float scale = 1.0f) {
        glm::vec3 ab = b - a;
        glm::vec3 ac = c - a;
        glm::vec3 n = glm::normalize(glm::cross(ab, ac));
        glm::vec3 center = (a + b + c) / 3.0f;
        DrawLine(center, center + n * scale, color);
    }


    void 
    FillTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 c, uint32_t color) {
        RendererCommand command;
        command.CommandType = CommandTypesDrawTriangles;
        command.DrawData.VertexStart = DebugRendererInstance.VertexCount;
        command.DrawData.VertexCount = 3;
        PushCommand(command);

        PushVertex(a, color, glm::vec2());
        PushVertex(b, color, glm::vec2());
        PushVertex(c, color, glm::vec2());
    }

    void
    DrawCircle(glm::vec3 center, float radius, glm::vec3 planeNormal, glm::vec3 planeTangent, uint32_t color) {
        float step = (glm::pi<float>() * 2.0f) / DEBUG_DRAW_CIRCLE_STEPS;
        glm::vec3 last = center + planeTangent * radius;
        for (int i = 1; i <= DEBUG_DRAW_CIRCLE_STEPS; ++i)
        {
            glm::vec3 next = center + glm::rotate(planeTangent, step * i, planeNormal) * radius;
            DrawLine(last, next, color);
            last = next;
        }
    }

    void
    DrawSphere(glm::vec3 center, float radius, uint32_t color) {
        DrawCircle(center, radius, glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), color);
        DrawCircle(center, radius, glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), color);
        DrawCircle(center, radius, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), color);
    }

    void
    FillBoxMinMax(glm::vec3 min, glm::vec3 max, uint32_t color) {
        glm::vec3 vertices[8];
        vertices[0] = glm::vec3(min.x, min.y, min.z);
        vertices[1] = glm::vec3(min.x, min.y, max.z);
        vertices[2] = glm::vec3(min.x, max.y, min.z);
        vertices[3] = glm::vec3(min.x, max.y, max.z);
        vertices[4] = glm::vec3(max.x, min.y, min.z);
        vertices[5] = glm::vec3(max.x, min.y, max.z);
        vertices[6] = glm::vec3(max.x, max.y, min.z);
        vertices[7] = glm::vec3(max.x, max.y, max.z);

        FillTriangle(vertices[0], vertices[1], vertices[2], color);
        FillTriangle(vertices[2], vertices[3], vertices[0], color);

        //FillTriangle(vertices[0], vertices[1], vertices[2], color);
        //FillTriangle(vertices[2], vertices[3], vertices[0], color);
    //
        //FillTriangle(vertices[0], vertices[1], vertices[2], color);
        //FillTriangle(vertices[2], vertices[3], vertices[0], color);
    //
        //FillTriangle(vertices[0], vertices[1], vertices[2], color);
        //FillTriangle(vertices[2], vertices[3], vertices[0], color);
    //
        //FillTriangle(vertices[0], vertices[1], vertices[2], color);
        //FillTriangle(vertices[2], vertices[3], vertices[0], color);
    //
        //FillTriangle(vertices[0], vertices[1], vertices[2], color);
        //FillTriangle(vertices[2], vertices[3], vertices[0], color);
    }

    void
    DrawBox(glm::vec3 center, glm::vec3 extents, uint32_t color) {
        glm::vec3 min = center - extents;
        glm::vec3 max = center + extents;
        DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, min.y, max.z), color);
        DrawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(min.x, max.y, max.z), color);
        DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z), color);
        DrawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), color);

        DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color);
        DrawLine(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color);
        DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color);
        DrawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color);

        DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), color);
        DrawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z), color);
        DrawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), color);
        DrawLine(glm::vec3(min.x, max.y, max.z), glm::vec3(max.x, max.y, max.z), color);
    }

    void
    DrawBoxMinMax(glm::vec3 min, glm::vec3 max, uint32_t color) {
        DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, min.y, max.z), color);
        DrawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(min.x, max.y, max.z), color);
        DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z), color);
        DrawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), color);

        DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color);
        DrawLine(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color);
        DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color);
        DrawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color);

        DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), color);
        DrawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z), color);
        DrawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), color);
        DrawLine(glm::vec3(min.x, max.y, max.z), glm::vec3(max.x, max.y, max.z), color);
    }

    inline uint32_t
    RGBAColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    	return a << 24 | b << 16 | g << 8 | r;
    }

    inline uint32_t
    RGBAColor(float r, float g, float b, float a) {
    	return RGBAColor((uint8_t)(r * 255.0f), (uint8_t)(g * 255.0f), (uint8_t)(b * 255.0f), (uint8_t)(a * 255.0f));
    }

    inline uint32_t
    RGBAColor(glm::vec4 color) {
        return RGBAColor(color.r, color.g, color.b, color.a);
    }

    inline uint32_t
    RGBAColorMultiplyAlpha(uint32_t color, float multiplier) {
    	float a = ((float)(color >> 24)) * multiplier;
    	return color | (((uint8_t)a) << 24);
    }

    void
    DrawCoordinateSystem(glm::vec3 pos, glm::quat rotation, glm::vec3 scale) {
        glm::vec3 right = glm::vec3(scale.x, 0, 0);
        glm::vec3 up = glm::vec3(0, scale.y, 0);
        glm::vec3 forward = glm::vec3(0, 0, scale.z);

        right = right * rotation;
        up = up * rotation;
        forward = forward * rotation;

        DrawLine(pos, pos + right, RGBAColor(1.0f, 0.0f, 0.0f, 1.0f));
        DrawLine(pos, pos + up, RGBAColor(0.0f, 1.0f, 0.0f, 1.0f));
        DrawLine(pos, pos + forward, RGBAColor(0.0f, 0.0f, 1.0f, 1.0f));
    }

    void
    DrawCoordinateSystem(glm::mat4 transformation) {
        glm::vec4 right = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec4 up = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        glm::vec4 forward = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

        right = transformation * right;
        up = transformation * up;
        forward = transformation * forward;

        glm::vec3 pos = glm::vec3(transformation[3]);

        DrawLine(pos, pos + glm::vec3(right), RGBAColor(1.0f, 0.0f, 0.0f, 1.0f));
        DrawLine(pos, pos + glm::vec3(up), RGBAColor(0.0f, 1.0f, 0.0f, 1.0f));
        DrawLine(pos, pos + glm::vec3(forward), RGBAColor(0.0f, 0.0f, 1.0f, 1.0f));
    }

    void
    DrawAxis(glm::vec3 pos, float scale, uint32_t color) {
        glm::vec3 right = glm::vec3(scale, 0, 0);
        glm::vec3 up = glm::vec3(0, scale, 0);
        glm::vec3 forward = glm::vec3(0, 0, scale);

        DrawLine(pos - right, pos + right, color);
        DrawLine(pos - up, pos + up, color);
        DrawLine(pos - forward, pos + forward, color);
    }

    glm::vec3 
    NormalizeScreenCoords(float x, float y) {
        return glm::vec3 (x * GLOBAL.InvScreenWidth * 2.0f - 1.0f,
                          y * GLOBAL.InvScreenHeight * 2.0f - 1.0f, 
                          0);
    }

    glm::vec3 
    NormalizeScreenCoordsFlipY(float x, float y) {
        return glm::vec3 (x * GLOBAL.InvScreenWidth * 2.0f - 1.0f,
                          -y * GLOBAL.InvScreenHeight * 2.0f + 1.0f, 
                          0);
    }

    void 
    FillScreenspaceQuad(float x, float y, float width, float height, float u0, float v0, float u1, float v1, uint32_t color) {
        
        glm::vec3 a = NormalizeScreenCoords(x, y);
        glm::vec3 b = NormalizeScreenCoords(x, y + height);
        glm::vec3 c = NormalizeScreenCoords(x + width, y + height);
        glm::vec3 d = NormalizeScreenCoords(x + width, y);

        glm::vec2 uva = glm::vec2(u0, v0);
        glm::vec2 uvb = glm::vec2(u0, v1);
        glm::vec2 uvc = glm::vec2(u1, v1);
        glm::vec2 uvd = glm::vec2(u1, v0);

        RendererCommand command;
        command.CommandType = CommandTypesDrawScreenSpaceTriangles;
        command.DrawData.VertexStart = DebugRendererInstance.VertexCount;
        command.DrawData.VertexCount = 6;
        PushCommand(command);

        PushVertex(a, color, uva);
        PushVertex(b, color, uvb);
        PushVertex(d, color, uvd);

        PushVertex(b, color, uvb);
        PushVertex(d, color, uvd);
        PushVertex(c, color, uvc);
    }

    void
    DebugRendererInstanceSetTexture(GLuint textureHandle) {
        RendererCommand command;
        command.CommandType = CommandTypesSetTexture;
        command.TextureData.TextureTarget = 0;
        command.TextureData.TextureHandle = textureHandle;
        PushCommand(command);
    }

    void
    AlphaBlending(bool enable) {
        RendererCommand command;
        command.CommandType = CommandTypesSetBlending;
        command.BlendingData.UseAlphaBlending = enable;
        PushCommand(command);
    }

    void 
    DrawDebugTexture(float x, float y, float width, float height, GLuint texture) {
        DebugRendererInstanceSetTexture(texture);
        AlphaBlending(false);

        glm::vec3 a = NormalizeScreenCoords(x, y);
        glm::vec3 b = NormalizeScreenCoords(x, y + height);
        glm::vec3 c = NormalizeScreenCoords(x + width, y + height);
        glm::vec3 d = NormalizeScreenCoords(x + width, y);

        glm::vec2 uva = glm::vec2(0, 0);
        glm::vec2 uvb = glm::vec2(0, 1);
        glm::vec2 uvc = glm::vec2(1, 1);
        glm::vec2 uvd = glm::vec2(1, 0);

        RendererCommand command;
        command.CommandType = CommandTypesDrawScreenSpaceTriangles;
        command.DrawData.VertexStart = DebugRendererInstance.VertexCount;
        command.DrawData.VertexCount = 6;
        PushCommand(command);

        PushVertex(a, 0xffffffff, uva);
        PushVertex(b, 0xffffffff, uvb);
        PushVertex(d, 0xffffffff, uvd);

        PushVertex(b, 0xffffffff, uvb);
        PushVertex(d, 0xffffffff, uvd);
        PushVertex(c, 0xffffffff, uvc);
        AlphaBlending(true);
    }



    enum renderer_state_culling {
    	renderer_state_culling_undefined,
    	renderer_state_culling_none,
    	renderer_state_culling_front,
    	renderer_state_culling_back
    };

    enum renderer_state_blending {
    	renderer_state_blending_undefined,
    	renderer_state_blending_none,
    	renderer_state_blending_alpha,
    	renderer_state_blending_add
    };

    enum renderer_state_depthtest {
    	renderer_state_depthtest_undefined,
    	renderer_state_depthtest_disabled,
    	renderer_state_depthtest_always,
    	renderer_state_depthtest_less,
    	renderer_state_depthtest_less_equal,
    	renderer_state_depthtest_equal,
    	renderer_state_depthtest_not_equal,
    	renderer_state_depthtest_greater,
    	renderer_state_depthtest_greater_equal,
    	renderer_state_depthtest_never
    };


    bool ScissorTest = false;
    int ScissorX = -1;
    int ScissorY = -1;
    int ScissorWidth = -1;
    int ScissorHeight = -1;

    GLuint ActiveTextures[8] = {(GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1, (GLuint)-1};
    renderer_state_culling Culling = renderer_state_culling_undefined;
    renderer_state_blending Blending = renderer_state_blending_undefined;
    renderer_state_depthtest DepthTest = renderer_state_depthtest_undefined;

    void
    SetScissorTest(bool enabled, int x = 0, int y = 0, int width = 0, int height = 0) {
        if(ScissorTest != enabled) {
            if(enabled){
                glEnable(GL_SCISSOR_TEST);
            } else {
                glDisable(GL_SCISSOR_TEST);
            }
            ScissorTest = enabled;
        }

        if(enabled && (ScissorX != x ||
                       ScissorY != y ||
                       ScissorWidth != width ||
                       ScissorHeight != height ))
        {
            glScissor(x, y, width, height);
        }
    }


    void
    SetTexture2D(int32_t textureIndex, GLuint texture) {
        if(ActiveTextures[textureIndex] != texture) { 
            glActiveTexture(GL_TEXTURE0 + textureIndex);
            glBindTexture(GL_TEXTURE_2D, texture);
            ActiveTextures[textureIndex] = texture;                 
        }
    }

    void
    SetCulling(renderer_state_culling culling) {
        if(Culling != culling) {
            switch(culling) {
                case renderer_state_culling_none: {
                    glDisable(GL_CULL_FACE);
                } break;
                case renderer_state_culling_front: {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                } break;
                case renderer_state_culling_back: {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                } break;
                default:
                    break;
            }
            Culling = culling;
        }
    }

    void
    SetBlending(renderer_state_blending blending) {
        if(Blending != blending) {
            switch(blending) {
                case renderer_state_blending_none: {
                    glDisable(GL_BLEND);
                } break;
                case renderer_state_blending_alpha: {
                    glEnable(GL_BLEND);
                    glBlendEquation(GL_FUNC_ADD);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                } break;
                case renderer_state_blending_add: {
                    glEnable(GL_BLEND);
                    glBlendEquation(GL_FUNC_ADD);
                    glBlendFunc(GL_ONE, GL_ONE);
                } break;
                default:
                    break;
            }
            
            Blending = blending;
        }
    }

    void
    SetDepthTest(renderer_state_depthtest depth) {
        if(DepthTest != depth) {
            switch(depth) {
                case renderer_state_depthtest_disabled: {
                    glDisable(GL_DEPTH_TEST);
                } break;
                case renderer_state_depthtest_never: {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_NEVER);
                } break;
                case renderer_state_depthtest_always: {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_ALWAYS);
                } break;
                case renderer_state_depthtest_less: {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LESS);
                } break;
                case renderer_state_depthtest_less_equal: {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LEQUAL);
                } break;
                case renderer_state_depthtest_equal: {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_EQUAL);
                } break;
                case renderer_state_depthtest_not_equal: {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_NOTEQUAL);
                } break;
                case renderer_state_depthtest_greater: {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_GREATER);
                } break;
                case renderer_state_depthtest_greater_equal: {
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_GEQUAL);
                } break;
                default:
                    break;
            }
            DepthTest = depth;
        }
    }


    void
    SubmitRenderer(glm::mat4x4 viewProjection) {
        if(!(DebugRendererInstance.Shader3D->IsValid && DebugRendererInstance.Shader2D->IsValid)) {
            return;
        }

        // Update all buffers.
        glBindBuffer(GL_ARRAY_BUFFER, DebugRendererInstance.VertexBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, DebugRendererInstance.VertexCount * sizeof(DebugVertex), DebugRendererInstance.Vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        ScissorTest = false;
        ScissorX = -1;
        ScissorY = -1;
        ScissorWidth = -1;
        ScissorHeight = -1;

        for(int i = 0; i < 8; ++i) {
            ActiveTextures[i] = (GLuint)-1;
        }
        Culling = renderer_state_culling_undefined;
        Blending = renderer_state_blending_undefined;
        DepthTest = renderer_state_depthtest_undefined;

        SetCulling(renderer_state_culling_none);
        SetBlending(renderer_state_blending_alpha);
        SetDepthTest(renderer_state_depthtest_less_equal);
        SetScissorTest(false);

        glBindVertexArray(DebugRendererInstance.VAO);

        for (int i = 0; i < DebugRendererInstance.CommandCount; ++i)
        {
            RendererCommand* command = DebugRendererInstance.Commands + i;
            switch(command->CommandType) {
                case CommandTypesDrawPoints: {
                    SetDepthTest(renderer_state_depthtest_less_equal);
                    DebugRendererInstance.Shader3D->Bind();
                    DebugRendererInstance.Shader3D->SetUniform("ViewProjection", viewProjection);
                    glDrawArrays(GL_POINTS, command->DrawData.VertexStart, command->DrawData.VertexCount);
                }  break;
                case CommandTypesDrawLines: {
                    SetDepthTest(renderer_state_depthtest_less_equal);
                    DebugRendererInstance.Shader3D->Bind();
                    DebugRendererInstance.Shader3D->SetUniform("ViewProjection", viewProjection);
                    glDrawArrays(GL_LINES, command->DrawData.VertexStart, command->DrawData.VertexCount);
                }  break;
                case CommandTypesDrawTriangles: {
                    SetDepthTest(renderer_state_depthtest_less_equal);
                    DebugRendererInstance.Shader3D->Bind();
                    DebugRendererInstance.Shader3D->SetUniform("ViewProjection", viewProjection);
                    glEnable(GL_POLYGON_OFFSET_FILL);
                    glPolygonOffset(1.0f, 1.0f);
                    glDrawArrays(GL_TRIANGLES, command->DrawData.VertexStart, command->DrawData.VertexCount);
                    glDisable(GL_POLYGON_OFFSET_FILL);
                }  break;
                case CommandTypesDrawScreenSpaceTriangles: {      
                    SetDepthTest(renderer_state_depthtest_disabled);
                    DebugRendererInstance.Shader2D->Bind();
                    glDrawArrays(GL_TRIANGLES, command->DrawData.VertexStart, command->DrawData.VertexCount);
                }  break;
                case CommandTypesSetTexture: {
                    SetTexture2D(command->TextureData.TextureTarget, command->TextureData.TextureHandle);
                }  break;
                case CommandTypesSetBlending: {
                    if(command->BlendingData.UseAlphaBlending) {
                        SetBlending(renderer_state_blending_alpha);
                    } else {
                        SetBlending(renderer_state_blending_none);
                    }
                }  break;
                case CommandTypesSetScissor: {
                    if(command->ScissorData.ScissorWidth == 0 || command->ScissorData.ScissorHeight == 0) {
                        SetScissorTest(false);
                    } else {
                        SetScissorTest(true, command->ScissorData.ScissorX, command->ScissorData.ScissorY, 
                                                     command->ScissorData.ScissorWidth, command->ScissorData.ScissorHeight);
                    }
                }  break;
                default:
                    break;
            }
        }

        // Reset some pipeline functions.
        SetBlending(renderer_state_blending_none);
        SetScissorTest(false);

        // Reset renderer for next frame.
        DebugRendererInstance.LastCommand = 0;
        DebugRendererInstance.CommandCount = 0;
        DebugRendererInstance.VertexCount = 0;
        if(DebugRendererInstance.TooManyVertices) {
            LogWarning("Debug Renderer: Too many vertices rendered!");
            DebugRendererInstance.TooManyVertices = false;
        }
        if(DebugRendererInstance.TooManyCommands) {
            LogWarning("Debug Renderer: Too many commands submitted!");
            DebugRendererInstance.TooManyCommands = false;
        }
    }
}