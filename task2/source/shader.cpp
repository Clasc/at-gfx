
struct SubShader
{
	std::string Name;
	std::string SourceFilePath;

	std::vector<std::string> Code;
	std::vector<std::string> FilePaths;

	int Type;
	uint32_t Shader = 0;
	bool IsValid = false;

	SubShader(std::string filePath, int type) {
		SourceFilePath = filePath;
		Type = type;
	}

	void
	PreprocessShaderCode(std::string shaderPath) {
		SDL_RWops *file = SDL_RWFromFile(shaderPath.c_str(), "r");
		if(!file) {
			LogWarning("Shader source file failed to open %s:\n%s\n", shaderPath.c_str(), SDL_GetError());
			return;
		}
		int64_t length = SDL_RWseek(file, 0, RW_SEEK_END);
		SDL_RWseek(file, 0, RW_SEEK_SET);

		char* sourceBuffer = new char[length + 1];
    	SDL_RWread(file, sourceBuffer, length, 1);
    	sourceBuffer[length] = 0;
    	SDL_RWclose(file);
    	std::string sourceCode = std::string(sourceBuffer);

    	size_t shaderPathDirectoryLength = shaderPath.find_last_of("/\\") + 1;
    	std::string shaderPathBaseDirectory = shaderPath.substr(0, shaderPathDirectoryLength);

    	size_t includePos = std::string::npos;
    	while((includePos = sourceCode.find("#include ")) != std::string::npos) {
    		std::string codeBefore = sourceCode.substr(0, includePos);
    		Code.push_back(codeBefore);

    		size_t includeEnd = sourceCode.find("\n", includePos);
    		if(includeEnd == std::string::npos) {
				LogWarning("Shader source file path include syntax error in file %s at code %.16s?", shaderPath.c_str(), sourceCode.substr(includePos, std::string::npos).c_str());
    			break;
    		}
    		std::string includeName = sourceCode.substr(includePos, includeEnd - includePos);
    		size_t pathStart = includeName.find_first_of("\"") + 1;
    		size_t pathEnd = includeName.find_last_of("\"");
    		includeName = includeName.substr(pathStart, pathEnd - pathStart);
    		std::string includePath = shaderPathBaseDirectory + includeName;

    		PreprocessShaderCode(includePath);

    		sourceCode = sourceCode.substr(includeEnd, std::string::npos);
    	}
    	Code.push_back(sourceCode);

    	delete[] sourceBuffer;
	}

	void Reload() {
		DeleteShader();
		Code.clear();

		Code.push_back("#version 400\n"
					   "#extension GL_ARB_explicit_attrib_location     : require\n"
					   "#extension GL_ARB_separate_shader_objects      : require\n"
		);

		if (Type == GL_COMPUTE_SHADER) {
			Code.push_back("#extension GL_ARB_compute_shader         : require\n");
		}
		#ifndef __APPLE__
			Code.push_back("#extension GL_ARB_shading_language_packing : require\n");
		#endif

		PreprocessShaderCode(SourceFilePath);
		if(Code.size() == 0) {
			IsValid = false;
			return;
		}
		char* shaderCodes[128];
		int shaderLengths[128];
		assert(Code.size() <= ArrayCount(shaderCodes));
		for(size_t i = 0; i < Code.size(); ++i) {
			shaderCodes[i] = (char*)Code[i].c_str();
			shaderLengths[i] = (int)Code[i].length();
		}

		GLint result = GL_FALSE;
		int infoLogLength;
		Shader = glCreateShader(Type);
		glShaderSource(Shader, (GLsizei)Code.size(), shaderCodes, shaderLengths);
		glCompileShader(Shader);
		glGetShaderiv(Shader, GL_COMPILE_STATUS, &result);
		glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		if(infoLogLength > 0) {
			char* error = new char[infoLogLength + 1];
			glGetShaderInfoLog(Shader, infoLogLength, NULL, error);
			LogWarning("Shader compilation failed for %s:\n%s\n", SourceFilePath.c_str(), error);
			delete[] error;
		}

		if(result == GL_TRUE) {
			IsValid = true;
		} else {
			glDeleteShader(Shader);
			IsValid = false;
		}
	}

	void DeleteShader() {
		if(IsValid) {
			glDeleteShader(Shader);
			IsValid = false;
		}
	}
};

struct Shader {
	std::string Name;
	SubShader* VertexShader = 0;
	SubShader* FragmentShader = 0;
	SubShader* TessellationControlShader = 0;
	SubShader* TessellationEvalShader = 0;
	SubShader* GeometryShader = 0;
	SubShader* ComputeShader = 0;

	uint32_t Program = (uint32_t)-1;
	bool IsValid = false;

	std::vector<std::string> ShaderCode;
	std::vector<std::string> ShaderFiles;

	std::unordered_map<std::string, uint32_t> Uniforms;

	static std::vector<Shader*> Shaders;
	static const uint32_t INVALID_LOCATION = (uint32_t)-1;

	Shader(char* name, char* vertexShaderPath = 0, char* fragmentShaderPath = 0, char* tessellationControlShaderPath = 0, char* tessellationEvalShaderPath = 0, char* geometryShaderPath = 0, char* computeShaderPath = 0) {
		Name = std::string(name);
		if(vertexShaderPath) {
			VertexShader = new SubShader(vertexShaderPath, GL_VERTEX_SHADER);
		}
		if(fragmentShaderPath) {
			FragmentShader = new SubShader(fragmentShaderPath, GL_FRAGMENT_SHADER);
		}
		if(tessellationControlShaderPath) {
			TessellationControlShader = new SubShader(tessellationControlShaderPath, GL_TESS_CONTROL_SHADER);
		}
		if(tessellationEvalShaderPath) {
			TessellationEvalShader = new SubShader(tessellationEvalShaderPath, GL_TESS_EVALUATION_SHADER);
		}
		if(geometryShaderPath) {
			GeometryShader = new SubShader(geometryShaderPath, GL_GEOMETRY_SHADER);
		}
		if(computeShaderPath) {
			ComputeShader = new SubShader(computeShaderPath, GL_COMPUTE_SHADER);
		}
		Reload();
		Shaders.push_back(this);
	}

	void 
	DeleteShader() {
		if(IsValid) {
			if(VertexShader && VertexShader->IsValid) {
				glDetachShader(Program, VertexShader->Shader);
			}

			if(TessellationControlShader && TessellationControlShader->IsValid) {
				glDetachShader(Program, TessellationControlShader->Shader);
			}
			
			if(TessellationEvalShader && TessellationEvalShader->IsValid) {
				glDetachShader(Program, TessellationEvalShader->Shader);
			}

			if(GeometryShader && GeometryShader->IsValid) {
				glDetachShader(Program, GeometryShader->Shader);
			}

			if(FragmentShader && FragmentShader->IsValid) {
				glDetachShader(Program, FragmentShader->Shader);
			}

			if(ComputeShader && ComputeShader->IsValid) {
				glDetachShader(Program, ComputeShader->Shader);
			}
			IsValid = false;
			glDeleteProgram(Program);
		}
		if(VertexShader) {
			VertexShader->DeleteShader();
		}

		if(TessellationControlShader) {
			TessellationControlShader->DeleteShader();
		}
		
		if(TessellationEvalShader) {
			TessellationEvalShader->DeleteShader();
		}

		if(GeometryShader) {
			GeometryShader->DeleteShader();
		}

		if(FragmentShader) {
			FragmentShader->DeleteShader();
		}

		if(ComputeShader) {
			ComputeShader->DeleteShader();
		}
	}


	void
	Reload() {
		DeleteShader();
		Uniforms.clear();
		Program = glCreateProgram();

		if(VertexShader) {
			VertexShader->Reload();
			if(VertexShader->IsValid) {
				glAttachShader(Program, VertexShader->Shader);
			}
		}

		if(TessellationControlShader) {
			TessellationControlShader->Reload();
			if(TessellationControlShader->IsValid) {
				glAttachShader(Program, TessellationControlShader->Shader);
			}
		}
		
		if(TessellationEvalShader) {
			TessellationEvalShader->Reload();
			if(TessellationEvalShader->IsValid) {
				glAttachShader(Program, TessellationEvalShader->Shader);
			}
		}

		if(GeometryShader) {
			GeometryShader->Reload();
			if(GeometryShader->IsValid) {
				glAttachShader(Program, GeometryShader->Shader);
			}
		}

		if(FragmentShader) {
			FragmentShader->Reload();
			if(FragmentShader->IsValid) {
				glAttachShader(Program, FragmentShader->Shader);
			}
		}

		if(ComputeShader) {
			ComputeShader->Reload();
			if(ComputeShader->IsValid) {
				glAttachShader(Program, ComputeShader->Shader);
			}
		}

		glLinkProgram(Program);

		GLint result = GL_FALSE;
		int infoLogLength;
		glGetProgramiv(Program, GL_LINK_STATUS, &result);
		glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0) {
			char* error = new char[infoLogLength + 1];
			glGetProgramInfoLog(Program, infoLogLength, NULL, error);
			LogWarning("Shader linking failed for Shader %s:\n%s\n", Name.c_str(), error);
			delete[] error;
		} 

		if(result == GL_TRUE) {
			GLint activeUniformCount = 0;
        	glGetProgramiv(Program, GL_ACTIVE_UNIFORMS, &activeUniformCount);
			GLint activeUniformMaxNameLength = 0;
        	glGetProgramiv(Program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &activeUniformMaxNameLength);
        	if(activeUniformCount > 0) {
	        	char* uniformName = new char[activeUniformMaxNameLength];

				for(int i = 0; i < activeUniformCount; ++i) {
					glGetActiveUniformName(Program, i, activeUniformMaxNameLength, 0, uniformName);
	        		uint32_t uniformLocation = glGetUniformLocation(Program, uniformName);
					Uniforms.insert({std::string(uniformName), uniformLocation});
					//LogMessage("    Added uniform %s with location %i.", uniformName, uniformLocation);
				}

				delete[] uniformName;
        	}

			IsValid = true;
		} else {
			glDeleteProgram(Program);
			IsValid = false;
		}
	}


	~Shader() {
		DeleteShader();
	    Shaders.erase(std::remove(Shaders.begin(), Shaders.end(), this));
	}

	void
	Bind() {
		if(IsValid) {
			glUseProgram(Program);
		}
	}

	uint32_t 
	GetUniformLocation(const char* uniformName) {
		auto it = Uniforms.find(std::string(uniformName));
		if(it == Uniforms.end()) {
			//LogWarning("Uniform %s was not found for shader %s!", uniformName, Name.c_str());
			return INVALID_LOCATION;
		} 
		return it->second;
	}

	void
	SetUniform(const char* uniformName, float value) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glUniform1f(location, value);
	}

	void
	SetUniform(const char* uniformName, glm::vec2 value) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glUniform2fv(location, 1, &value[0]);
	}

	void
	SetUniform(const char* uniformName, glm::vec3 value) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glUniform3fv(location, 1, &value[0]);
	}

	void
	SetUniform(const char* uniformName, glm::vec4 value) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glUniform4fv(location, 1, &value[0]);
	}

	void
	SetUniform(const char* uniformName, uint32_t value) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glUniform1ui(location, value);
	}

	void
	SetUniform(const char* uniformName, int32_t value) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glUniform1i(location, value);
	}

	void
	SetUniform(const char* uniformName, glm::mat4 value) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
	}

	void
	SetUniform(const char* uniformName, glm::mat4* value, int count) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glUniformMatrix4fv(location, count, GL_FALSE, (float*)value);
	}

	void 
	SetTexture(const char* uniformName, uint32_t slot, Texture* texture) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, texture ? texture->OpenGLBuffer : 0);
		
		glUniform1i(location, slot);
	}
	
	void 
	SetTexture(const char* uniformName, uint32_t slot, uint32_t textureBuffer) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, textureBuffer);
		
		glUniform1i(location, slot);
	}

	void 
	SetTextureArray(const char* uniformName, uint32_t slot, uint32_t textureBuffer) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textureBuffer);
		
		glUniform1i(location, slot);
	}

	void 
	SetTextureBuffer(const char* uniformName, uint32_t slot, uint32_t textureBuffer) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_BUFFER, textureBuffer);
		
		glUniform1i(location, slot);
	}

	void
	SetTexture(const char* uniformName, Texture* texture) {
		uint32_t location = GetUniformLocation(uniformName);
		if(location == INVALID_LOCATION) {
			return;
		}
		if(texture && texture->IsBindless) {
			glUniformHandleui64ARB(location, texture->BindlessHandle);
		} else {
			glUniformHandleui64ARB(location, 0);
		}
	}
};

std::vector<Shader*> Shader::Shaders = std::vector<Shader*>();
