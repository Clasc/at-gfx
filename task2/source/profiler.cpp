#define PROFILING_MAXFRAMES 10
#define PROFILING_MAXCPUQUERIES 100
#define PROFILING_MAXGPUQUERIES 100
#define PROFILING_GPUFRAMELATENCY 5

struct QueryResult
{
    uint32_t frame;
    uint64_t startTime;
    uint64_t endTime; 
    uint64_t elapsedTime;
    
    uint64_t startTimeCPU;
    uint64_t endTimeCPU;
};

struct Query
{
    const char* name;
    int32_t index;
    uint32_t lastFrameIndex;
    uint32_t currentFrameIndex;
    
    QueryResult frameResults[PROFILING_MAXFRAMES];
};

struct QueryCPU : Query
{
    uint64_t startTime;
};

struct QueryGPU : Query
{
    uint32_t frameHardwareQueriesStart[PROFILING_MAXFRAMES];
    uint32_t frameHardwareQueriesEnd[PROFILING_MAXFRAMES];
    uint64_t startTimeCPU[PROFILING_MAXFRAMES];
    uint64_t endTimeCPU[PROFILING_MAXFRAMES];
};

struct Profiler
{
    uint32_t currentFrame;
    uint32_t currentFrameIndex;
    uint32_t newQueryIndexCPU;
    uint32_t newQueryIndexGPU;
    
    double CPUPerformanceFrequencyMSInv;
    double GPUPerformanceFrequencyMSInv;
    bool GPUPerformanceFrequencyQueryFirstDone;
    
    QueryCPU CPUQueries[PROFILING_MAXCPUQUERIES];
    QueryGPU GPUQueries[PROFILING_MAXGPUQUERIES];
    
    uint64_t frameStartTimeCPU[PROFILING_MAXFRAMES];
    uint64_t frameEndTimeCPU[PROFILING_MAXFRAMES];
    
    void Initialize() {
        this->currentFrame = (uint32_t)-1;
        this->currentFrameIndex = (uint32_t)-1;
        this->newQueryIndexCPU = 0;
        this->newQueryIndexGPU = 0;
        this->CPUPerformanceFrequencyMSInv = 1000.0 / (double)SDL_GetPerformanceFrequency();
        this->GPUPerformanceFrequencyMSInv = 1.0 / 1000000.0; // Fixed to nanoseconds in OGL.
        
        this->GPUPerformanceFrequencyQueryFirstDone = false;
        for (size_t f = 0; f < PROFILING_MAXFRAMES; f++) {
            this->frameStartTimeCPU[f] = 0;
            this->frameEndTimeCPU[f] = 0;
        }
        
        for (size_t i = 0; i < PROFILING_MAXCPUQUERIES; i++) {
            this->CPUQueries[i].name = 0;
            this->CPUQueries[i].lastFrameIndex = 0;
            this->CPUQueries[i].currentFrameIndex = 0;
            this->CPUQueries[i].index = -1;
            for (size_t f = 0; f < PROFILING_MAXFRAMES; f++) {
                this->CPUQueries[i].frameResults[f].frame = (uint32_t)-1;
            }
        }
        for (size_t i = 0; i < PROFILING_MAXGPUQUERIES; i++) {
            this->GPUQueries[i].name = 0;
            this->GPUQueries[i].lastFrameIndex = 0;
            this->GPUQueries[i].currentFrameIndex = 0;
            this->GPUQueries[i].index = -1;
            for (size_t f = 0; f < PROFILING_MAXFRAMES; f++) {
                this->GPUQueries[i].frameResults[f].frame = (uint32_t)-1;
                this->GPUQueries[i].frameHardwareQueriesStart[f] = 0;
                this->GPUQueries[i].frameHardwareQueriesEnd[f] = 0;
                this->GPUQueries[i].startTimeCPU[f] = 0;
                this->GPUQueries[i].endTimeCPU[f] = 0;
            }
        }
    }
    
    void NewFrame() {
        if (this->GPUPerformanceFrequencyQueryFirstDone) {
            this->frameEndTimeCPU[this->currentFrameIndex] = SDL_GetPerformanceCounter();
        }
        
        this->currentFrameIndex++;
        this->currentFrameIndex %= PROFILING_MAXFRAMES;
        this->currentFrame++;
        
        this->frameStartTimeCPU[this->currentFrameIndex] = SDL_GetPerformanceCounter();
        
        this->GPUPerformanceFrequencyQueryFirstDone = true;
    }
    
    QueryCPU* StartCPUQuery(const char* name){
        int index = -1;
        // Compare the constant pointer to the name string to find the query if it already exists.
        for (uint32_t i = 0; i < this->newQueryIndexCPU; i++) {
            if (this->CPUQueries[i].name == name) {
                index = i;
                break;
            }
        }
        
        // If none exists, try to create a new one.
        if (index == -1) {
            index = this->newQueryIndexCPU;
            if (index <= PROFILING_MAXCPUQUERIES) {
                this->CPUQueries[index].index = this->newQueryIndexCPU;
                this->CPUQueries[index].name = name;
                this->newQueryIndexCPU++;
            } else {
                SDL_Log("Maximum [%i] of CPU performance queries reached!", PROFILING_MAXCPUQUERIES);
                return 0;
            }
        }
        
        // Restart the query.
        this->CPUQueries[index].startTime = SDL_GetPerformanceCounter();
        
        // Return the address to the query.
        return &(this->CPUQueries[index]);
    }
    
    void 
        StopCPUQuery(QueryCPU* query)
    {
        if (query == 0)
            return;
        
        QueryResult* result = &query->frameResults[query->currentFrameIndex];
        result->startTime = query->startTime;
        result->endTime = SDL_GetPerformanceCounter();
        result->startTimeCPU = result->startTime;
        result->endTimeCPU = result->endTime;
        
        if (result->frame != this->currentFrame) {
            // Reset elapsed time.
            result->elapsedTime = result->endTime - result->startTime;
        } 
        else {
            // Extend elapsed time.
            result->elapsedTime += result->endTime - result->startTime;
        }
        
        result->frame = this->currentFrame;
        
        query->lastFrameIndex = query->currentFrameIndex;
        query->currentFrameIndex++;
        if (query->currentFrameIndex >= PROFILING_MAXFRAMES) {
            query->currentFrameIndex = 0;
        }
    }
    
    QueryGPU* 
        StartGPUQuery(const char* name)
    {
        int index = -1;
        // Compare the constant pointer to the name string to find the query if it already exists.
        for (uint32_t i = 0; i < this->newQueryIndexGPU; i++) {
            if (this->GPUQueries[i].name == name) {
                index = i;
                break;
            }
        }
        
        // If none exists, try to create a new one.
        if (index == -1) {
            index = this->newQueryIndexGPU;
            if (index <= PROFILING_MAXGPUQUERIES) {
                this->GPUQueries[index].index = this->newQueryIndexGPU;
                this->GPUQueries[index].name = name;
                this->newQueryIndexGPU++;
            } else {
                SDL_Log("Maximum [%i] of GPU performance queries reached!", PROFILING_MAXGPUQUERIES);
                return 0;
            }
        }
        
        // Create or restart the query.
        QueryGPU* query = &this->GPUQueries[index];
        if (query->frameHardwareQueriesStart[query->currentFrameIndex] == 0) {
            glGenQueries(1, &query->frameHardwareQueriesStart[query->currentFrameIndex]);
        }
		glQueryCounter(query->frameHardwareQueriesStart[query->currentFrameIndex], GL_TIMESTAMP);
        
        // Store cpu start time to create a frame graph.
        query->startTimeCPU[query->currentFrameIndex] = SDL_GetPerformanceCounter();
        
        // Return the address to the query.
        return &(this->GPUQueries[index]);
    }
    
    void 
        StopGPUQuery(QueryGPU* query)
    {
        if (query == 0)
            return;
        
        // Stop the query for this frame.            
        if (query->frameHardwareQueriesEnd[query->currentFrameIndex] == 0) {
            glGenQueries(1, &query->frameHardwareQueriesEnd[query->currentFrameIndex]);
        }
		glQueryCounter(query->frameHardwareQueriesEnd[query->currentFrameIndex], GL_TIMESTAMP);
        
        // Store cpu end time to create a frame graph.
        query->endTimeCPU[query->currentFrameIndex] = SDL_GetPerformanceCounter();
        
        // Get result for query PROFILING_GPUFRAMELATENCY ago.
        int resultFrameIndex = query->currentFrameIndex - PROFILING_GPUFRAMELATENCY;
        if (resultFrameIndex < 0) {
            resultFrameIndex += PROFILING_MAXFRAMES;
        }
        
        if (query->frameHardwareQueriesStart[resultFrameIndex] && query->frameHardwareQueriesEnd[resultFrameIndex]) {
            QueryResult* result = &query->frameResults[resultFrameIndex];
            result->startTimeCPU = query->startTimeCPU[resultFrameIndex];
            result->endTimeCPU = query->endTimeCPU[resultFrameIndex];
            result->frame = this->currentFrame - PROFILING_GPUFRAMELATENCY;
            
            int hasResult = GL_FALSE;
            while(hasResult == GL_FALSE) {
            	glGetQueryObjectiv(query->frameHardwareQueriesStart[resultFrameIndex], GL_QUERY_RESULT_AVAILABLE, &hasResult);
            }
            glGetQueryObjectui64v(query->frameHardwareQueriesStart[resultFrameIndex], GL_QUERY_RESULT, &result->startTime);
			
			hasResult = GL_FALSE;
            while(hasResult == GL_FALSE) {
            	glGetQueryObjectiv(query->frameHardwareQueriesEnd[resultFrameIndex], GL_QUERY_RESULT_AVAILABLE, &hasResult);
            }
            glGetQueryObjectui64v(query->frameHardwareQueriesEnd[resultFrameIndex], GL_QUERY_RESULT, &result->endTime);

            result->elapsedTime = result->endTime - result->startTime;
        }
        
        query->lastFrameIndex = resultFrameIndex;
        query->currentFrameIndex++;
        if (query->currentFrameIndex >= PROFILING_MAXFRAMES) {
            query->currentFrameIndex = 0;
        }
    }
    
    void Draw() {
        if (ImGui::CollapsingHeader("CPU")) {
            for (size_t i = 0; i < this->newQueryIndexCPU; i++) {
                QueryResult* result = &this->CPUQueries[i].frameResults[this->CPUQueries[i].lastFrameIndex];
                float val = (float)(((double)(result->elapsedTime)) * this->CPUPerformanceFrequencyMSInv);
                ImGui::Value(this->CPUQueries[i].name, val);
            }
        }
        
        if (ImGui::CollapsingHeader("GPU")) {
            for (size_t i = 0; i < this->newQueryIndexGPU; i++) {
                QueryResult* result = &this->GPUQueries[i].frameResults[this->GPUQueries[i].lastFrameIndex];
                float val = (float)(((double)(result->elapsedTime)) * this->GPUPerformanceFrequencyMSInv);
                ImGui::Value(this->GPUQueries[i].name, val);
            }
        }
        
        // Get the results for the first valid GPU frame.
        int resultFrameIndex = this->currentFrameIndex - PROFILING_GPUFRAMELATENCY;
        if (resultFrameIndex < 0) {
            resultFrameIndex += PROFILING_MAXFRAMES;
        }
        
        //ImGui::Begin("Frame Timing");
        //
        //float windowWidth = ImGui::GetWindowWidth();
        //float frameSize = windowWidth / PROFILING_MAXFRAMES;
        //float hSize = 10.0f;
        //
        //ImGui::Columns(PROFILING_MAXFRAMES, 0, true);
        //for (size_t i = 0; i < PROFILING_MAXFRAMES; i++) {
        //    ImGui::Text("Frame %i", this->currentFrame - PROFILING_MAXFRAMES + i);
        //    ImGui::NextColumn();
        //}
        //for (size_t i = 0; i < this->newQueryIndexCPU; i++) {
        //    QueryResult* result = &this->CPUQueries[i].frameResults[this->CPUQueries[i].lastFrameIndex];
        //
        //    ImGui::SameLine();
        //    ImGui::Spacing();
        //}
        //
        //ImGui::Spacing(100);
        //
        //ImGui::SameLine();
        //ImGui::Button("Test 50", ImVec2(50, 10));
        //
        //ImGui::SameLine();
        //ImGui::Spacing(200);
        //
        //ImGui::SameLine();
        //ImGui::Button("Test 100", ImVec2(100, 10));
        
        //ImGui::End();
    }
};

Profiler GlobalProfiler;