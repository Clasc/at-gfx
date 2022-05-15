// Disable some warnings on cl.exe
#define _CRT_SECURE_NO_WARNINGS

// Included standard library stuff
#include <cstdio>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <list>
#include <time.h>
#include <chrono>
#include <thread>
#include <string>
#include <cstring>

// Include glew for OpenGL extensions.
#include "../external/GL/glew.h"

// Include SDL headers for platform abstraction.
#include "../external/SDL/SDL.h"
#include "../external/SDL/SDL_video.h"
#include "../external/SDL/SDL_opengl.h"
#include "../external/SDL/SDL_syswm.h"

// Include glm math functions and disable some warnings for them
// we use warning=error so even minor warnings will break the build.
#pragma warning( push )
#pragma warning( disable : 4201)
#define GLM_FORCE_RADIANS
#include "../external/glm/glm.hpp"
#include "../external/glm/gtc/matrix_transform.hpp"
#include "../external/glm/gtc/constants.hpp"
#include "../external/glm/gtc/reciprocal.hpp"
#include "../external/glm/gtc/type_ptr.hpp"
#include "../external/glm/gtx/rotate_vector.hpp"
#include "../external/glm/gtc/epsilon.hpp"
#include "../external/glm/gtc/noise.hpp"
#include "../external/glm/gtc/random.hpp"
#include "../external/glm/gtx/norm.hpp"
#include "../external/glm/gtx/hash.hpp"
#include "../external/glm/gtx/intersect.hpp"
#pragma warning( pop )

// Include imgui for gui display.
#pragma warning( push )
#pragma warning( disable : 4456)
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#include "../external/imgui.h"
#include "../external/imgui.cpp"
#include "../external/imgui_widgets.cpp"
#include "../external/imgui_draw.cpp"
#include "../external/imgui_impl_opengl3.cpp"
#include "../external/imgui_impl_sdl.cpp"
#pragma warning( pop )

// Include tinygltf for GLTF 2.0 loading.
#pragma warning( push )
#pragma warning( disable : 4267)
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#include "tiny_gltf.h"
#pragma warning( pop )

// Include to generate pseudo random series for TAA.
#pragma warning( push )
#pragma warning( disable : 4244)
#include "../external/halton_sampler.h"
#pragma warning( pop )

// Used for resizing images to fit into the texture array.
#pragma warning( push )
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../external/stb_image_resize.h"
#pragma warning( pop )

// Define some basic macros that we will use in the code.
#define ArrayCount(arr) (sizeof(arr) / sizeof(arr[0]))
#define BufferOffset(i) ((char *)0 + (i))

// Define some global data for easier use.
struct {
    int MouseX;
    int MouseY;
    int ScreenWidth;
    int ScreenHeight;
    float InvScreenWidth;
    float InvScreenHeight;
    float DeltaTime;
    char* GPUVendor;
    char* GPUModel;
} GLOBAL;

// Include the base header for shaders. This will be included in every shader too.
#include "../shaders/base.h"

// Include our engine code.
#include "platform.cpp"
#include "profiler.cpp"
#include "log.cpp"
#include "input.cpp"
#include "camera.cpp"
#include "texture.cpp"
#include "rendertarget.cpp"
#include "shader.cpp"
#include "debug_renderer.cpp"
#include "scene.cpp"
#include "bvh.cpp"
#include "scene_renderer.cpp"


int main(int argc, char *argv[])
{
    // Initialize global values.
    GLOBAL.ScreenWidth = 800; 
    GLOBAL.ScreenHeight = 600;
    GLOBAL.InvScreenWidth = 1.0f / (float)GLOBAL.ScreenWidth;
    GLOBAL.InvScreenHeight = 1.0f / (float)GLOBAL.ScreenHeight;

    // Initialize SDL and OpenGL.
    SDL_Window *Window;
    bool IsRunning;
    SDL_GLContext GLContext;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        LogError("Initialization error: %s", SDL_GetError());
        return -1;
    }

    // Set backbuffer to be SRGB8.
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Add a depth buffer for our GUI and debug rendering.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Ask for an OpenGL 4.0 core profile.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Enable debug context when in debug mode for more OpenGL debug output.
    #if DEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    #endif

    // Create the window.
    Window = SDL_CreateWindow("[rrt engine]", 100, 100, GLOBAL.ScreenWidth, GLOBAL.ScreenHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!Window) {
        LogError("Window creation error: %s", SDL_GetError());
        return -1;
    }

    // Create the OpenGL context.
    GLContext = SDL_GL_CreateContext(Window);
    if (!GLContext) {
        LogError("OpenGL context creation error: %s", SDL_GetError());
        return -1;
    } else {
        // If we failed try to create the highest possible GL version.
        int major, minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        LogMessage("OpenGL context initialized with version: %i.%i", major, minor);
    }

    // Disable Vertical Sync to get framerates > display refresh rate.
    bool enableVsync = false;
    SDL_GL_SetSwapInterval(enableVsync ? 1 : 0);

    // Initialize the OpenGL extensions via glew.
    GLenum glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK) {
        LogError("GLEW initialization error: %s\n",  (char*)glewGetErrorString(glewInitResult));
        return -1;
    }
    ClearGLErrors();

    // Get the name of the active GPU and output it (interesting for multi-GPU setups).
    GLOBAL.GPUVendor = (char*)glGetString(GL_VENDOR); 
    GLOBAL.GPUModel = (char*)glGetString(GL_RENDERER); 
    LogMessage("Used graphics adapter: %s %s", GLOBAL.GPUVendor, GLOBAL.GPUModel);

    // Enable additional debug output for OpenGL when in debug build.
    #if DEBUG
        glDebugMessageCallback((GLDEBUGPROC)&GLDebugCallback, 0);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    #endif

    // Setup ImGui binding for SDL.
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(Window, GLContext);
    ImGui_ImplOpenGL3_Init();

    // Initialize our debug renderer.
    Debug::InitializeRenderer();

    // Create our scene renderer.
    SceneRenderer* sceneRenderer = new SceneRenderer(GLOBAL.ScreenWidth, GLOBAL.ScreenHeight);

    // Create our camera.
    Camera* camera = new Camera();
    camera->AspectRatio = (float)GLOBAL.ScreenWidth / (float)GLOBAL.ScreenHeight;
    camera->Direction = glm::vec3(1.0f, 0.0f, 0.0f);
    camera->Farplane = 1000.0f;
    camera->Nearplane = 0.1f;
    camera->FieldOfView = glm::radians(42.1f);
    camera->Position = glm::vec3(0.0f, 2.5f, -1.50f);
    camera->LookAt(glm::vec3(2.0f, 0.0f, 1.1f));
    camera->UpdateProjectionMatrix();
    camera->UpdateViewMatrix();

    // Create our scene and load from gltf file.
    Scene* scene = new Scene("../../data/Fireplace/Fireplace.gltf", false);
    Node* lightNode = new Node();
    //lightNode->Position = glm::vec3(1.8f, 2.2f, -1.8f);
    lightNode->Position = glm::vec3(-0.1f, 2.3f, -0.3f);
    glm::vec3 direction = glm::normalize(glm::vec3(3, -5, -3));
    lightNode->Rotation = glm::quatLookAt(direction, glm::vec3(0, 1, 0));
    lightNode->Scale = glm::vec3(1, 1, 1);
    scene->RootNode->Children.push_back(lightNode);
    Light* light = new Light();
    light->ParentNode = lightNode;
    light->Type = LightTypePoint;
    light->Intensity = 100.0f;
    light->Radius = 0.1f;
    light->Range = 15.0f;
    light->Color = glm::vec3(1, 1, 1);
    light->InnerAngle = 0.3f;
    light->OuterAngle = 0.5f;

    scene->Lights.push_back(light);

    // Generate bvh.
    BVH* bvh = new BVH();
    bvh->GenerateBVH(scene);
    bool drawBVH = false;


    GlobalProfiler.Initialize();

    uint64_t currentTime = SDL_GetPerformanceCounter();
    uint64_t counterFrequency = SDL_GetPerformanceFrequency();
    float deltaTime = 0.01f;

    SDL_Event event;
    IsRunning = true;

    float Frametimes[RENDERING_FRAMETIMES_COUNT];
    int CurrentFrametimeIndex = 0;

    while(IsRunning) {

        // Compute the last frame time.
        uint64_t newTime = SDL_GetPerformanceCounter();
        uint64_t deltaTimeTicks = newTime - currentTime;
        deltaTime = glm::max(0.001f, (float)((double)deltaTimeTicks / (double)counterFrequency));

        if(enableVsync) {
            float targetMS = 1000.0f / 60.0f;
            float deltaDiff = targetMS - (deltaTime * 1000.0f);
            while(deltaDiff > 1.5f) {
                SDL_Delay((int)deltaDiff);
                newTime = SDL_GetPerformanceCounter();
                deltaTimeTicks = newTime - currentTime;
                deltaTime = glm::max(0.001f, (float)((double)deltaTimeTicks / (double)counterFrequency));
                deltaDiff = targetMS - (deltaTime * 1000.0f);
            }
        }

        currentTime = newTime;
        GLOBAL.DeltaTime = deltaTime;

        Frametimes[CurrentFrametimeIndex] = deltaTime * 1000.0f;
        CurrentFrametimeIndex++;
        if(CurrentFrametimeIndex >= RENDERING_FRAMETIMES_COUNT) {
            CurrentFrametimeIndex = 0;
        }

        // Clear input for this frame before SDL events are dispatched.
        Input::NewFrame();
        GlobalProfiler.NewFrame();
        
        // SDL event loop.
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            Input::Update(event);
            if (event.type == SDL_QUIT) {
                IsRunning = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    float aspect = (float)event.window.data1 / (float)event.window.data2;
                    camera->AspectRatio = aspect;
                    camera->UpdateProjectionMatrix();

                    GLOBAL.ScreenWidth = event.window.data1; 
                    GLOBAL.ScreenHeight = event.window.data2;
                    GLOBAL.InvScreenWidth = 1.0f / (float)GLOBAL.ScreenWidth;
                    GLOBAL.InvScreenHeight = 1.0f / (float)GLOBAL.ScreenHeight;
                    
                    sceneRenderer->Resize(GLOBAL.ScreenWidth, GLOBAL.ScreenHeight);
                }
            } else if(event.type == SDL_MOUSEMOTION) {
                GLOBAL.MouseX = event.motion.x;
                GLOBAL.MouseY = event.motion.y;
            }
        }

        // Call new frame handlers for ImGui.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(Window);
        ImGui::NewFrame();

        
        camera->NewFrame(GLOBAL.ScreenWidth, GLOBAL.ScreenHeight, false);

        // Camera movement code.
        if (Input::IsMouseButtonPressed(SDL_BUTTON_RIGHT)) {
            camera->RotateX(Input::GetMouseMovementY() * 0.01f);
            camera->RotateY(Input::GetMouseMovementX() * 0.01f);

            float camSpeed = 1.0f;
            if (Input::IsKeyPressed(SDL_SCANCODE_LSHIFT)) {
                camSpeed = 20.0f;
            }
            if (Input::IsKeyPressed(SDL_SCANCODE_W)) {
                camera->MoveForward(camSpeed * deltaTime);
            }
            if (Input::IsKeyPressed(SDL_SCANCODE_S)) {
                camera->MoveForward(-camSpeed * deltaTime);
            }
            if (Input::IsKeyPressed(SDL_SCANCODE_A)) {
                camera->MoveLeft(camSpeed * deltaTime);
            }
            if (Input::IsKeyPressed(SDL_SCANCODE_D)) {
                camera->MoveLeft(-camSpeed * deltaTime);
            }
        }

        // Statistics window.
        ImGui::Begin("Statistics");
        ImGui::Text("FPS: %i", (int)(1.0f / GLOBAL.DeltaTime));
        ImGui::Text("Camera Pos: %f %f %f", camera->Position.x, camera->Position.y, camera->Position.z);
        ImGui::Text("Scene Meshes: %i", (int)scene->Meshes.size());
        ImGui::Text("BVH Nodes: %i", bvh->BVHNodeCount);
        static int nodeLevelsDrawn = 8;
        ImGui::Checkbox("Draw BVH", &drawBVH);
        if(drawBVH) {
            ImGui::InputInt("BVH Levels Drawn", &nodeLevelsDrawn);
        }
        GlobalProfiler.Draw();
        ImGui::End();

        ImGui::Begin("Rendering");
        ImGui::SliderFloat("Exposure", &camera->Exposure, -10.0f, 10.0f);
        ImGui::Text("Frametime: %f", GLOBAL.DeltaTime * 1000.0f);
        ImGui::PlotHistogram("Frametimes", Frametimes, RENDERING_FRAMETIMES_COUNT, 0, 0, 0, 50.0, ImVec2(0.0f, 100.0f));

        if(ImGui::Checkbox("Enable VSync", &enableVsync)) {
            SDL_GL_SetSwapInterval(enableVsync ? 1 : 0);
        }
        ImGui::End();

        ImGui::Begin("Light");
        ImGui::DragFloat3("Position", &lightNode->Position[0], 0.1f);
        glm::vec3 lightEuler = glm::degrees(glm::eulerAngles(lightNode->Rotation));
        if(ImGui::DragFloat3("Rotation", &lightEuler[0])) {
            lightNode->Rotation = glm::quat(glm::radians(lightEuler));
        }
        char* LightTypes[] = {"Point", "Spot", "Directional"};
        if(ImGui::BeginCombo("Type", LightTypes[light->Type])) {
            for(int i = 0; i < ArrayCount(LightTypes); ++i) {
                if(ImGui::Selectable(LightTypes[i])) {
                    light->Type = (LightType)i;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::ColorEdit3("Color", &light->Color[0]);
        ImGui::SliderFloat("Intensity", &light->Intensity, 0.0f, 10000.0f);
        ImGui::SliderFloat("Radius", &light->Radius, 0.0f, 1.0f);
        ImGui::SliderFloat("Range", &light->Range, 0.0f, 500.0f);
        float innerAngleDeg = glm::degrees(light->InnerAngle);
        if(ImGui::SliderFloat("InnerAngle", &innerAngleDeg, 0.0f, 180.0f)) {
            light->InnerAngle = glm::radians(innerAngleDeg);
            if(light->InnerAngle > light->OuterAngle) {
                light->OuterAngle = light->InnerAngle;
            }
        }
        float outerAngleDeg = glm::degrees(light->OuterAngle);
        if(ImGui::SliderFloat("OuterAngle", &light->OuterAngle, 0.0f, 180.0f)) {
            light->OuterAngle = glm::radians(outerAngleDeg);
            if(light->OuterAngle < light->InnerAngle) {
                light->InnerAngle = light->OuterAngle;
            }
        }
        ImGui::End();

        ImGui::Begin("Render Targets");
        float targetWidth = ImGui::GetContentRegionAvailWidth();
        char nameBuffer[2048];
        for(auto const& renderTarget: RenderTarget::RenderTargets) {
            sprintf_s(nameBuffer, ArrayCount(nameBuffer), "RT [");
            for (int i = 0; i < 8; ++i)
            {  
                if(renderTarget->Layers[i]) {
                    if(i != 0) {
                        strcat_s(nameBuffer, ArrayCount(nameBuffer), ",");
                    }
                    strcat_s(nameBuffer, ArrayCount(nameBuffer), renderTarget->Layers[i]->Name);
                }
            }
            strcat_s(nameBuffer, ArrayCount(nameBuffer), "]");

            if(ImGui::CollapsingHeader(nameBuffer)) {
                for (int i = 0; i < 8; ++i) {
                    if(renderTarget->Layers[i]) {
                        Texture* texture = renderTarget->Layers[i]->TargetTexture;
                        float aspect = (float)texture->Height / (float)texture->Width;
                        ImGui::Text("%s", renderTarget->Layers[i]->Name);
                        ImVec4 tint = ImVec4(1, 1, 1, 1);
                        ImGui::Image((ImTextureID)(uint64_t)texture->OpenGLBuffer, ImVec2(targetWidth, targetWidth * aspect), ImVec2(0, 1), ImVec2(1, 0), tint);
                    }
                }
            }
        }
        ImGui::End();


        ImGui::Begin("Shaders");
        ImGui::Columns(2);
        for(auto const& shader: Shader::Shaders) {
            ImGui::PushID(shader);
            ImGui::Text("%s", shader->Name.c_str());
            ImGui::NextColumn();
            if(ImGui::Button("Reload")) {
                shader->Reload();
            }
            ImGui::NextColumn();
            ImGui::PopID();
        }
        ImGui::Columns(1);
        ImGui::End();

        if(drawBVH) {
            bvh->Draw(nodeLevelsDrawn);
        }

        Debug::DrawCoordinateSystem(glm::vec3(), glm::quat(), glm::vec3(1, 1, 1));
        
        glClearDepth(1.0f);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        scene->UpdateNodes();
        sceneRenderer->Draw(scene, camera, bvh);
        //added denoising filter
        sceneRenderer->Display();

        Debug::SubmitRenderer(camera->ViewProjection);

        // Generate the ImGui render commands and submit them to OpenGL.
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Display whole frame.
        SDL_GL_SwapWindow(Window);
    }

    // Cleanup Imgui and SDL.
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(GLContext); 
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}