struct Texture {
	int32_t Width;
	int32_t Height;
	int32_t MipLevels;
	GLuint Format;
	GLuint Wrap;
	GLuint Filter;
	uint8_t MSAASamples;

	bool IsValid = false;
	bool IsBindless = false;

	GLuint OpenGLBuffer;
	GLuint64 BindlessHandle;
	
	Texture(int32_t width, int32_t height, int32_t mipLevels = -1, GLuint format = GL_RGBA8_SNORM, uint8_t msaaSamples = 1, GLuint wrap = GL_REPEAT, GLuint filter = GL_LINEAR, bool bindless = false) {
		Format = format;
		Width = width;
		Height = height;
		MipLevels = mipLevels;
		Wrap = wrap;
		Filter = filter;
		MSAASamples = msaaSamples;
		IsBindless = bindless;
	}

	bool IsCompressed() {
		switch(Format) {
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			case GL_COMPRESSED_RED_RGTC1:
			case GL_COMPRESSED_SIGNED_RED_RGTC1:
			case GL_COMPRESSED_SIGNED_RG_RGTC2:
			case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
				return true;
			default:
				return false;
		}
	}

	int GetBitsPerPixel(GLuint dataFormat, GLuint dataType) {
		int multiplier = 1;
		switch(dataType) {
			case GL_RED:
				multiplier = 1;
			case GL_RG:
				multiplier = 2;
			case GL_RGB:
			case GL_BGR:
				multiplier = 3;
			case GL_RGBA:
			case GL_BGRA:
				multiplier = 4;
			case GL_RED_INTEGER:
			case GL_RG_INTEGER:
			case GL_RGB_INTEGER:
			case GL_BGR_INTEGER:
			case GL_RGBA_INTEGER:
			case GL_BGRA_INTEGER: {
				multiplier = 1;
				LogWarning("Image format is not supported, it may or may not work!");
			}
			case GL_STENCIL_INDEX:
			case GL_DEPTH_COMPONENT:
			case GL_DEPTH_STENCIL:
				multiplier = 1;
			default:
				multiplier = 1;
		}

		switch (dataFormat) {
			case GL_UNSIGNED_BYTE:
			case GL_BYTE:
				return multiplier * 8;
			case GL_UNSIGNED_SHORT:
			case GL_SHORT:
				return multiplier * 16;
			case GL_UNSIGNED_INT:
			case GL_INT:
				return multiplier * 32;
			case GL_HALF_FLOAT:
				return multiplier * 16;
			case GL_FLOAT:
				return multiplier * 32;
			case GL_UNSIGNED_BYTE_3_3_2:
			case GL_UNSIGNED_BYTE_2_3_3_REV:
				return 8;
			case GL_UNSIGNED_SHORT_5_6_5:
			case GL_UNSIGNED_SHORT_5_6_5_REV:
				return 16;
			case GL_UNSIGNED_SHORT_4_4_4_4:
			case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			case GL_UNSIGNED_SHORT_5_5_5_1:
			case GL_UNSIGNED_SHORT_1_5_5_5_REV:
				return 16;
			case GL_UNSIGNED_INT_8_8_8_8:
			case GL_UNSIGNED_INT_8_8_8_8_REV:
			case GL_UNSIGNED_INT_10_10_10_2:
			case GL_UNSIGNED_INT_2_10_10_10_REV:
				return 32;
			default:
				return multiplier * 8;
		}
	}

	void
	UpdateData(uint8_t* data = 0, GLuint dataFormat = GL_RGBA, GLuint dataType = GL_UNSIGNED_BYTE) {
		uint32_t texTarget = GL_TEXTURE_2D;
		if(MSAASamples > 1) {
			texTarget = GL_TEXTURE_2D_MULTISAMPLE;
		}

		bool computeMips = false;
		if(MipLevels == -1) {
			MipLevels = (int32_t)glm::floor(log2(glm::max(Width, Height))) + 1;
			computeMips = true;
		}

		glGenTextures(1, &OpenGLBuffer);
	    glBindTexture(texTarget, OpenGLBuffer);
	 	if(MSAASamples > 1) {
	        glTexImage2DMultisample(texTarget, MSAASamples, Format, Width, Height, false);
	        glTexParameteri(texTarget, GL_TEXTURE_BASE_LEVEL, 0);
	        glTexParameteri(texTarget, GL_TEXTURE_MAX_LEVEL, 0);
	    } else {
	        glTexStorage2D(texTarget, MipLevels, Format, Width, Height);
	        glTexParameteri(texTarget, GL_TEXTURE_BASE_LEVEL, 0);
	        glTexParameteri(texTarget, GL_TEXTURE_MAX_LEVEL, MipLevels - 1);
	        
	        glTexParameteri(texTarget, GL_TEXTURE_WRAP_S, Wrap);
	        glTexParameteri(texTarget, GL_TEXTURE_WRAP_T, Wrap);
	        glTexParameteri(texTarget, GL_TEXTURE_WRAP_R, Wrap);

		    glTexParameterf(texTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
	        glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, Filter == GL_NEAREST ? GL_NEAREST : GL_LINEAR);
	        glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, Filter);

	        if(data) {
		        int dataMipLevels = computeMips ? 1 : MipLevels;
				int bitsPerPixel = GetBitsPerPixel(dataFormat, dataType);
				int levelWidth = Width;
				int levelHeight = Height;
				uint8_t* levelData = data;
				for (int i = 0; i < dataMipLevels; ++i) {
					int levelSize = 0;
					if(IsCompressed()) {
						int blocksX = glm::max(1, levelWidth / 4);
						int blocksY = glm::max(1, levelHeight / 4);
						int blockSize = 1;

						if(Format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || 
						   Format == GL_COMPRESSED_SRGB_S3TC_DXT1_EXT ||
						   Format == GL_COMPRESSED_RED_RGTC1 || 
						   Format == GL_COMPRESSED_SIGNED_RED_RGTC1)
						{
							blockSize = 8;
						} else {
							blockSize = 16;
						}

						levelSize = blocksX * blocksY * blockSize;
						glCompressedTexSubImage2D(texTarget, i, 0, 0, levelWidth, levelHeight, Format, levelSize, levelData);
					} else {
						levelSize = (levelWidth * levelHeight * bitsPerPixel) / 8;
						glTexSubImage2D(texTarget, i, 0, 0, levelWidth, levelHeight, dataFormat, dataType, levelData);
					}
					levelData += levelSize;
					levelWidth /= 2;
					levelHeight /= 2;
				}

				if(computeMips) {
					glGenerateMipmap(texTarget);
				}
			}
	    }
		
	    if(IsBindless) {
			BindlessHandle = glGetTextureHandleARB(OpenGLBuffer);
			glMakeTextureHandleResidentARB(BindlessHandle);
	    }

	    glBindTexture(texTarget, 0);

	    IsValid = true;
	}

	void
	ReplaceData(uint8_t* data = 0, GLuint dataFormat = GL_RGBA, GLuint dataType = GL_UNSIGNED_BYTE, int width = 0, int height = 0) {
		glBindTexture(GL_TEXTURE_2D, OpenGLBuffer);
		if(width == 0) {
			width = Width;
		}
		if(height == 0) {
			height = Height;
		}
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, dataFormat, dataType, data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void GenerateMipmap() {
	    glBindTexture(GL_TEXTURE_2D, OpenGLBuffer);
		glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	}

	void ReadbackTexture(int level, void* data, GLuint dataFormat = GL_RGBA, GLuint dataType = GL_UNSIGNED_BYTE) {
		glBindTexture(GL_TEXTURE_2D, OpenGLBuffer);
		glGetTexImage(GL_TEXTURE_2D, level, dataFormat, dataType, data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	~Texture() {
		if(OpenGLBuffer) {
			glDeleteTextures(1, &OpenGLBuffer);
		}
	}
};

Texture* LoadTextureFromFile(char* path) {
	int width, height, components;
	uint8_t* data = stbi_load(path, &width, &height, &components, 0);
	if(!data ) {
		LogWarning("...failed to load texture from file %s\n", path);
		return 0;
	} else {
		GLuint format = GL_RGBA8_SNORM;
		if(components == 3) {
			format = GL_RGB8_SNORM;
		} else if (components == 1) {
			format = GL_R8;
		}
		Texture* texture = new Texture(width, height, -1, format);
		texture->UpdateData(data, format);
		stbi_image_free(data);
		return texture;
	}
}