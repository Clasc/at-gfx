
struct SceneRenderer {
	Shader* PBRShader;
    Shader* GBufferShader;
    Shader* PostProcessingShader;

    uint32_t LightBuffer = 0;
    uint32_t LightBufferTexture = 0;    
    int LightBufferCount = 0;

    RenderTargetLayer* GBufferDepth;
    RenderTargetLayer* GBufferNormal;
    RenderTargetLayer* GBufferAlbedoTransparency;
    RenderTargetLayer* GBufferMetalnessRoughness;
    RenderTargetLayer* GBufferMotion;
    RenderTarget* GBuffer;

    RenderTarget* IntermediateBuffer;
    RenderTargetLayer* IntermediateBufferColor;

    RenderTarget* MainBuffer;
    RenderTargetLayer* MainBufferColor;
    RenderTargetLayer* MainBufferDepth;

    uint32_t FullscreenVAO = 0;
    uint32_t FrameCount = 0;

	SceneRenderer(int width, int height) {
		PBRShader = new Shader("PBR", "../../shaders/pbr.vert", "../../shaders/pbr.frag");
        GBufferShader = new Shader("GBuffer", "../../shaders/gbuffer.vert", "../../shaders/gbuffer.frag");
        PostProcessingShader = new Shader("Post Processing", "../../shaders/post.vert", "../../shaders/post.frag");
        
        GBufferDepth = new RenderTargetLayer(width, height, 1, GL_DEPTH24_STENCIL8, 1, GL_CLAMP_TO_EDGE, GL_NEAREST, "GBuffer Depth");
        GBufferNormal = new RenderTargetLayer(width, height, 1, GL_RG16F, 1, GL_CLAMP_TO_EDGE, GL_NEAREST, "GBuffer Normal");
        GBufferAlbedoTransparency = new RenderTargetLayer(width, height, 1, GL_RGBA8, 1, GL_CLAMP_TO_EDGE, GL_NEAREST, "GBuffer Albedo Transparency");
        GBufferMetalnessRoughness = new RenderTargetLayer(width, height, 1, GL_RGB10_A2, 1, GL_CLAMP_TO_EDGE, GL_NEAREST, "GBuffer Metalness Roughness");
        GBufferMotion = new RenderTargetLayer(width, height, 1, GL_RG16F, 1, GL_CLAMP_TO_EDGE, GL_NEAREST, "GBuffer Motion");
        RenderTargetLayer* gBufferTargets[] = {GBufferNormal, GBufferAlbedoTransparency, GBufferMetalnessRoughness, GBufferMotion}; 
        GBuffer = new RenderTarget(GBufferDepth, ArrayCount(gBufferTargets), gBufferTargets);

        IntermediateBufferColor = new RenderTargetLayer(width, height, 1, GL_RGBA16F, 1, GL_CLAMP_TO_EDGE, GL_LINEAR, "Intermediate Buffer");
        IntermediateBuffer = new RenderTarget(0, 1, &IntermediateBufferColor);
                        
        MainBufferColor = new RenderTargetLayer(width, height, 1, GL_RGBA8, 1, GL_CLAMP_TO_EDGE, GL_LINEAR, "Main Color Buffer");
        MainBuffer = new RenderTarget(GBufferDepth, 1, &MainBufferColor);

        glGenVertexArrays(1, &FullscreenVAO);
	}

    void Resize(int width, int height) {
        if(width < 2) {
            width = 2;
        }
        if(height < 2) {
            height = 2;
        }

        GBuffer->Resize(width, height);
        IntermediateBuffer->Resize(width, height);
        MainBuffer->Resize(width, height);
    }

    void DrawMesh(Scene* scene, Mesh* mesh) {
        if(mesh->VAO == 0) {
            return;
        }
        
        glBindVertexArray(mesh->VAO);

        size_t groupCount = mesh->Groups.size();
        int boundMaterial = -1;
        for (size_t i = 0; i < groupCount; ++i) {
            Group group = mesh->Groups[i];
        	if(group.MaterialIndex != boundMaterial) {
        		if(group.MaterialIndex != -1) {
                    GBufferShader->SetUniform("MaterialIndex", (int)group.MaterialIndex);
        		} else {
                    GBufferShader->SetUniform("MaterialIndex", (int)0);
                }
        		boundMaterial = group.MaterialIndex; 
        	}
            glDrawElements(GL_TRIANGLES, group.IndexCount, GL_UNSIGNED_INT, (void*)(size_t)(group.IndexStart * sizeof(uint32_t)));
        }
        glBindVertexArray(0);
    }

    void DrawNode(Scene* scene, Node* node) {
        GBufferShader->SetUniform("World", node->WorldMatrix);
        GBufferShader->SetUniform("WorldInvTrans", node->WorldMatrixInverseTranspose);
        if (node->LinkedMesh) {
            DrawMesh(scene, node->LinkedMesh);
        }
        for (size_t i = 0; i < node->Children.size(); i++) {
            DrawNode(scene, node->Children[i]);
        }
    }

    void Draw(Scene* scene, Camera* camera, BVH* bvh) {
        if(scene->IsValid && GBufferShader->IsValid && PBRShader->IsValid) {
            auto gpuGBuffer = GlobalProfiler.StartGPUQuery("GBuffer");
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);

            // Draw GBuffer
            GBuffer->Clear(true, true, true, 1.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            GBuffer->Bind();

            UpdateLights(scene);
            GBufferShader->Bind();
            GBufferShader->SetUniform("ViewProjection", camera->ViewProjection);
            GBufferShader->SetUniform("ViewProjectionOld", camera->ViewProjectionOld);   
            GBufferShader->SetTextureBuffer("MaterialBuffer", 0, scene->MaterialBufferTexture);
            GBufferShader->SetTextureArray("MaterialTextures", 1, scene->TextureArray);

            DrawNode(scene, scene->RootNode);
            GBuffer->Unbind();

            GlobalProfiler.StopGPUQuery(gpuGBuffer);

            auto gpuComputeLighting = GlobalProfiler.StartGPUQuery("Compute Lighting");

            // Draw to lighting buffers.
            Shader* pbr = PBRShader;
            
            glDepthFunc(GL_ALWAYS);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glDepthMask(GL_FALSE);

            pbr->Bind();
            pbr->SetUniform("LightCount", LightBufferCount);
            pbr->SetUniform("FrameCount", FrameCount);
            pbr->SetUniform("CameraPosition", camera->Position);
            pbr->SetUniform("CameraInvViewProjection", camera->ViewProjectionInv);
            pbr->SetUniform("RenderingScale", glm::vec2(1.0f, 1.0f));

            // Be careful with the texture slots because some intel cards may only have valid slots from 0 to 7
            pbr->SetTexture("GBufferDepth", 0, GBufferDepth->TargetTexture);
            pbr->SetTexture("GBufferNormal", 1, GBufferNormal->TargetTexture);
            pbr->SetTexture("GBufferAlbedoTransparency", 2, GBufferAlbedoTransparency->TargetTexture);
            pbr->SetTexture("GBufferMetallnessRoughness", 3, GBufferMetalnessRoughness->TargetTexture);
            pbr->SetTextureBuffer("LightBuffer", 4 ,LightBufferTexture);
            pbr->SetTextureBuffer("MaterialBuffer", 5, scene->MaterialBufferTexture);
            pbr->SetTextureArray("MaterialTextures", 6, scene->TextureArray);

            IntermediateBuffer->Bind();
            glBindVertexArray(FullscreenVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
            IntermediateBuffer->Unbind();

            GlobalProfiler.StopGPUQuery(gpuComputeLighting);

            auto gpuTonemap = GlobalProfiler.StartGPUQuery("Tonemap");
            // Draw to main buffer.
            MainBuffer->Bind();
            PostProcessingShader->Bind();
            PostProcessingShader->SetUniform("RenderingScale", glm::vec2(1, 1));
            PostProcessingShader->SetUniform("RenderingScaleOld", glm::vec2(1, 1));
            PostProcessingShader->SetUniform("CameraExposure", camera->Exposure);
            PostProcessingShader->SetTexture("IntermediateBuffer", 0, IntermediateBufferColor->TargetTexture);
            glBindVertexArray(FullscreenVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
            MainBuffer->Unbind();
            GlobalProfiler.StopGPUQuery(gpuTonemap);
        }
        ++FrameCount;
    }

    void UpdateLights(Scene* scene) {
        if(LightBufferCount < scene->Lights.size()) {
            LightBufferCount = (int)scene->Lights.size();
            if(LightBuffer) {
                glDeleteBuffers(1, &LightBuffer);
                glDeleteTextures(1, &LightBufferTexture);
            }
            glGenBuffers(1, &LightBuffer);
            glBindBuffer(GL_TEXTURE_BUFFER, LightBuffer);
            glBufferData(GL_TEXTURE_BUFFER, sizeof(RendererLight) * LightBufferCount, 0, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_TEXTURE_BUFFER, 0);

            glGenTextures(1, &LightBufferTexture);
            glBindTexture(GL_TEXTURE_BUFFER, LightBufferTexture);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, LightBuffer);
            glBindTexture(GL_TEXTURE_BUFFER, 0);
        }

        glBindBuffer(GL_TEXTURE_BUFFER, LightBuffer);
        RendererLight* mappedLightBuffer = (RendererLight*)glMapBuffer(GL_TEXTURE_BUFFER, GL_WRITE_ONLY);
        for(int i = 0; i < LightBufferCount; ++i) {
            Light* light = scene->Lights[i];
            Node* parentNode = light->ParentNode;
            if(parentNode) {
                mappedLightBuffer->Position = parentNode->Position;
                glm::quat rot = parentNode->Rotation;
                mappedLightBuffer->Direction = -glm::vec3(2.0f * (rot.x * rot.z + rot.w * rot.y), 2.0f * (rot.y * rot.z - rot.w * rot.x), 1.0f - 2.0f * (rot.x * rot.x + rot.y * rot.y));
            }
            mappedLightBuffer->Range = light->Range;
            mappedLightBuffer->Type = light->Type;
            mappedLightBuffer->Color = light->Color;
            mappedLightBuffer->Intensity = light->Intensity;
            mappedLightBuffer->Radius = light->Radius;
            mappedLightBuffer->AngleScale = 1.0f / glm::max(0.001f, glm::cos(light->InnerAngle) - glm::cos(light->OuterAngle));
            mappedLightBuffer->AngleOffset = -glm::cos(light->OuterAngle) * mappedLightBuffer->AngleScale;

            ++mappedLightBuffer;
        }
        glUnmapBuffer(GL_TEXTURE_BUFFER);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

    void Display() {
        BlitRenderTarget(MainBuffer, 0, true);
    }
};