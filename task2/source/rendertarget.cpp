
struct RenderTargetLayer {
	Texture* TargetTexture;
	uint8_t MSAACount;
	char Name[256];

	RenderTargetLayer(Texture* texture, uint8_t samples, char* name) {
		TargetTexture = texture;
		MSAACount = samples;
		strcpy_s(Name, ArrayCount(Name), name);
	}

	RenderTargetLayer(int32_t width, int32_t height, int32_t mipLevels, GLuint format, uint8_t samples, GLuint wrap, GLuint filter, char* name) {
		TargetTexture = new Texture(width, height, mipLevels, format, samples, wrap, filter);
		TargetTexture->UpdateData();
		MSAACount = samples;
		strcpy_s(Name, ArrayCount(Name), name);
	}

	void
	Resize(int32_t width, int32_t height) {
		if(TargetTexture->Width != width || TargetTexture->Height != height) {
			Texture* texture = new Texture(width, height, TargetTexture->MipLevels, TargetTexture->Format, MSAACount, TargetTexture->Wrap, TargetTexture->Filter);	
			texture->UpdateData();
			delete TargetTexture;
			TargetTexture = texture;
		}
	}

	~RenderTargetLayer() {
		delete TargetTexture;
	}
};

struct RenderTarget {
	uint32_t FBO;
	RenderTargetLayer* DepthBuffer;
	uint8_t LayerCount;
	RenderTargetLayer* Layers[8];
	GLenum DrawBuffers[8];

	static std::vector<RenderTarget*> RenderTargets;

	void
	_RecreateRenderTarget() {
		if(FBO) {
	        glDeleteFramebuffers(1, &FBO);
	    }

	    glGenFramebuffers(1, &FBO);
	    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	 
		if(DepthBuffer) {
			if(DepthBuffer->MSAACount > 1) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, DepthBuffer->TargetTexture->OpenGLBuffer, 0);
			} else {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, DepthBuffer->TargetTexture->OpenGLBuffer, 0);
			}
		}

		for (int32_t i = 0; i < LayerCount; ++i) {
			RenderTargetLayer* layer = Layers[i];

			if(layer->MSAACount > 1) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D_MULTISAMPLE, layer->TargetTexture->OpenGLBuffer, 0);
			} else {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, layer->TargetTexture->OpenGLBuffer, 0 );
			}
			DrawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
		}

	    glDrawBuffers(LayerCount, DrawBuffers);

	  	GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	    if(result != GL_FRAMEBUFFER_COMPLETE) {
	        LogError("Failed to create frame buffer!\n");
			if (result == GL_FRAMEBUFFER_UNDEFINED) {
				LogError("... default framebuffer does not exist!\n");
			}
			if (result == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
				LogError("... any framebuffer attachement point is incomplete!\n");
			}
			if (result == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
				LogError("... attachements are missing!\n");
			}
			if (result == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER) {
				LogError("... draw buffer incomplete!\n");
			}
			if (result == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) {
				LogError("... read buffer incomplete!\n");
			}
			if (result == GL_FRAMEBUFFER_UNSUPPORTED) {
				LogError("... attachement combination is unsupported!\n");
			}
			if (result == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE) {
				LogError("... not all attachement have the same sample count!\n");
			}
			if (result == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS) {
				LogError("... attachement is layered but there is a problem with it!\n");
			}
	    }

	    glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}



	RenderTarget(RenderTargetLayer* depthBuffer, uint8_t layerCount, RenderTargetLayer** layers) 
	{
		FBO = 0;
		LayerCount = layerCount;
		DepthBuffer = depthBuffer;
		for (int32_t i = 0; i < layerCount; ++i) {
			Layers[i] = layers[i];
		}
		for (int32_t i = layerCount; i < ArrayCount(Layers); ++i) {
			Layers[i] = 0;
		}

		RenderTargets.push_back(this);

		_RecreateRenderTarget();
	}

	~RenderTarget() {
		if(FBO) {
	        glDeleteFramebuffers(1, &FBO);
	    }
	    RenderTargets.erase(std::remove(RenderTargets.begin(), RenderTargets.end(), this));
	}

	void
	Resize(int32_t width, int32_t height) {
		if(DepthBuffer) {
			DepthBuffer->Resize(width, height);
		}

		for (uint8_t i = 0; i < LayerCount; ++i) {
			RenderTargetLayer* layer = Layers[i];

			layer->Resize(width, height);
		}

		_RecreateRenderTarget();
	}

	void
	Bind(float viewportScaleX = 1.0f, float viewportScaleY = 1.0f) {
		RenderTargetLayer* layer = Layers[0];
		if(layer) {
			if(layer->MSAACount > 1) {
	    		glEnable(GL_MULTISAMPLE);
			} else {
	    		glDisable(GL_MULTISAMPLE);
			}
	    	glViewport(0, 0, (int)((float)layer->TargetTexture->Width * viewportScaleX), (int)((float)layer->TargetTexture->Height * viewportScaleY));
		}		

	    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	    glDrawBuffers(LayerCount, DrawBuffers);
	}

	
	void ReadbackTexture(void* data, GLuint dataFormat = GL_RGBA, GLuint dataType = GL_UNSIGNED_BYTE, int layer = 0, int width = 0, int height = 0) {
	    glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
		glReadBuffer(GL_COLOR_ATTACHMENT0 + layer);
		glReadPixels(0, 0, width, height, dataFormat, dataType, data);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}


	void Clear(bool clearDepth = true, bool clearColor = true, bool clearStencil = true, float depth = 1.0f, glm::vec4 color = glm::vec4(0), uint8_t stencil = 0) {
		Bind();

		GLbitfield mask = 0;
		if(clearDepth) {
			glClearDepth(1.0f);
			mask |= GL_DEPTH_BUFFER_BIT;
		}
		if(clearColor) {
        	glClearColor(color.r, color.g, color.b, color.a);
			mask |= GL_COLOR_BUFFER_BIT;
		}
		if(clearStencil) {
			glClearStencil(stencil);
			mask |= GL_STENCIL_BUFFER_BIT;
		}

        glClear(mask);

		Unbind();
	}


	void Unbind() {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
};

std::vector<RenderTarget*> RenderTarget::RenderTargets = std::vector<RenderTarget*>();


void
BlitRenderTarget(RenderTarget* source, RenderTarget* target, bool blitDepthStencil = false, bool blitAllLayers = false, int sourceWidth = 0, int sourceHeight = 0) {
	int targetWidth = 0;
	int targetHeight = 0;

	if(source) {
		RenderTargetLayer* layer = source->Layers[0];
		if(layer) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, source->FBO); 

			if(layer->TargetTexture) {
				if(sourceWidth == 0) {
					sourceWidth = layer->TargetTexture->Width;	
				}

				if(sourceHeight == 0) {
					sourceHeight = layer->TargetTexture->Height;
				}
			} else {
				LogWarning("Source rendertarget texture of layer 0 not valid.");
			}
		} else {
			LogWarning("Source rendertarget layer 0 not valid.");
		}
	} else {
		if(sourceWidth == 0) {
			sourceWidth = GLOBAL.ScreenWidth;
		}
		if(sourceHeight == 0) {
			sourceHeight = GLOBAL.ScreenHeight;
		}
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	if(target) {
		RenderTargetLayer* layer = target->Layers[0];
		if(layer) {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->FBO); 
			if(!blitAllLayers) {
				GLenum firstLayer = GL_COLOR_ATTACHMENT0;
	    		glDrawBuffers(1, &firstLayer);
			}
			if(layer->TargetTexture) {
				if(blitDepthStencil) {
					targetWidth = glm::min(layer->TargetTexture->Width, sourceWidth);	
					targetHeight = glm::min(layer->TargetTexture->Height, sourceHeight);
				} else {
					targetWidth = layer->TargetTexture->Width;
					targetHeight = layer->TargetTexture->Height;
				}
			} else {
				LogWarning("Target rendertarget texture of layer 0 not valid.");
			}
		} else {
			LogWarning("Target rendertarget layer 0 not valid.");
		}
	} else {
		if(blitDepthStencil) {
			targetWidth = glm::min(GLOBAL.ScreenWidth, sourceWidth);
			targetHeight =  glm::min(GLOBAL.ScreenHeight, sourceHeight); 
		} else {
			targetWidth = GLOBAL.ScreenWidth;
			targetHeight = GLOBAL.ScreenHeight;
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);
	}

	GLbitfield mask = GL_COLOR_BUFFER_BIT;
	if(blitDepthStencil) {
		mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	}
	GLint filter = GL_NEAREST;
	if(!blitDepthStencil && (sourceWidth != targetWidth || sourceHeight != targetHeight)) {
		filter = GL_LINEAR;
	}
	glBlitFramebuffer(0, 0, sourceWidth, sourceHeight, 0, 0, targetWidth, targetHeight, mask, filter);
}

