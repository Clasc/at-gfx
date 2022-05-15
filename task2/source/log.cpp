
#define LOG_MAX_MESSAGE_LENGTH 400

enum LogSeverity {
	LogSeverityInfo,
	LogSeverityWarning,
	LogSeverityError
};

static FILE* GlobalLogFile = 0;

void
LogMessageGeneric(LogSeverity severity, const char* message, ...) {
	if(!GlobalLogFile) {
		GlobalLogFile = fopen("rrt.log", "wb+");
	}
	if(message) {
		va_list args;
	    va_start(args, message);
	    vprintf(message, args);
	    vfprintf(GlobalLogFile, message, args);
	    va_end(args);
	    
	    printf("\n");
	    fflush(GlobalLogFile);
	}
}

#if _MSC_VER
#define LogMessage(a, ...) LogMessageGeneric(LogSeverityInfo, a, __VA_ARGS__)
#define LogWarning(a, ...) LogMessageGeneric(LogSeverityWarning,  a, __VA_ARGS__)
#define LogError(a, ...) LogMessageGeneric(LogSeverityError,  a, __VA_ARGS__)
#else
#define LogMessage(a, ...) LogMessageGeneric(LogSeverityInfo, a, ##__VA_ARGS__)
#define LogWarning(a, ...) LogMessageGeneric(LogSeverityWarning,  a, ##__VA_ARGS__)
#define LogError(a, ...) LogMessageGeneric(LogSeverityError,  a, ##__VA_ARGS__)
#endif


void GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, void *userParam) {
    const char* sourceName = "NONE";
    switch(source) {
        case GL_DEBUG_SOURCE_API: {
            sourceName = "API";
        } break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: {
            sourceName = "WINDOW";
        } break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: {
            sourceName = "SHADER";

            // Ignore shader messages as we get them directly at compile.
            return;
        } break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: {
            sourceName = "THIRDPARTY";
        } break;
        case GL_DEBUG_SOURCE_APPLICATION: {
            sourceName = "APP";
        } break;
        case GL_DEBUG_SOURCE_OTHER: {
            sourceName = "OTHER";
        } break;
    }

    const char* typeName = "NONE";
    switch(type) {
        case GL_DEBUG_TYPE_ERROR: {
            typeName = "ERROR";
        } break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {
            typeName = "DEPRECATED";
        } break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: {
            typeName = "UB";
        } break;
        case GL_DEBUG_TYPE_PORTABILITY: {
            typeName = "PORT";
        } break;
        case GL_DEBUG_TYPE_PERFORMANCE: {
            typeName = "PERF";
        } break;
        case GL_DEBUG_TYPE_MARKER: {
            typeName = "MARKER";
        } break;
        case GL_DEBUG_TYPE_PUSH_GROUP: {
            typeName = "GROUP_PUSH";
        } break;
        case GL_DEBUG_TYPE_POP_GROUP: {
            typeName = "GROUP_POP";
        } break;
        case GL_DEBUG_TYPE_OTHER: {
            typeName = "OTHER";
        } break;
    } 

    const char* severityName = "NONE";
    switch(severity) {
        case GL_DEBUG_SEVERITY_LOW: {
            severityName = "LOW";
        } break;
        case GL_DEBUG_SEVERITY_MEDIUM: {
            severityName = "MEDIUM";
        } break;
        case GL_DEBUG_SEVERITY_HIGH: {
            severityName = "HIGH";
        } break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: {
            severityName = "NOTE";
            // Ignore notes (there are so many for some drivers).
            return;
        } break;
        case GL_DONT_CARE: {
            severityName = "NONE";
        } break;
    }


    LogMessage("GL (%s, %s, %s): %s", sourceName, typeName, severityName, message);
}

void
_LogGLError(const char* file, int32_t line) {
    GLenum errorID (glGetError());
 
    while(errorID!=GL_NO_ERROR) {
        const char* error = "UNKNOWN";

        switch(errorID) {
            case GL_INVALID_OPERATION:{  
                error = "INVALID_OPERATION";      
            } break;
            case GL_INVALID_ENUM: {
                error = "INVALID_ENUM";
            } break;
            case GL_INVALID_VALUE: {
                error = "INVALID_VALUE";
            } break;
            case GL_OUT_OF_MEMORY: {
                error = "OUT_OF_MEMORY";
            } break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: {
                error = "INVALID_FRAMEBUFFER_OPERATION";  
            } break;
        }

        LogError("GL Error %s at %s:%i\n", error, file, line);
        errorID = glGetError();
   }
}

void
ClearGLErrors() {
    GLenum errorID (glGetError());
    while(errorID != GL_NO_ERROR) {
        errorID = glGetError();
   }
}

#define CheckGLError() _LogGLError(__FILE__, __LINE__); 
