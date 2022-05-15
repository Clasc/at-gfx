namespace Input
{
    bool Input_KeysPressed[283];
    bool Input_MouseButtonPressedThisFrame[8];
    bool Input_MouseButtonUpThisFrame[8];
    bool Input_MouseButtonPressed[8];
    float Input_MouseMovementX;
    float Input_MouseMovementY;

    void 
    NewFrame()
    {
        Input_MouseMovementX = 0;
        Input_MouseMovementY = 0;

        for (int i = 0; i < 8; ++i) {
            Input_MouseButtonPressedThisFrame[i] = false;
            Input_MouseButtonUpThisFrame[i] = false;
        }
    }

    void 
    Update(SDL_Event event)
    {
        if (event.type == SDL_KEYDOWN){
            Input_KeysPressed[event.key.keysym.scancode] = true;
        }
        else if (event.type == SDL_KEYUP){
            Input_KeysPressed[event.key.keysym.scancode] = false;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN){
            Input_MouseButtonPressed[event.button.button] = true;
            Input_MouseButtonPressedThisFrame[event.button.button] = true;
        }
        else if (event.type == SDL_MOUSEBUTTONUP){
            Input_MouseButtonPressed[event.button.button] = false;
            Input_MouseButtonUpThisFrame[event.button.button] = true;
        }
        else if (event.type == SDL_MOUSEMOTION){
            Input_MouseMovementX = (float)event.motion.xrel;
            Input_MouseMovementY = (float)event.motion.yrel;
        }
    }

    bool 
    IsKeyPressed(SDL_Scancode key)
    {
        return Input_KeysPressed[key];
    }

    bool 
    IsMouseButtonPressed(uint8_t button)
    {
        return Input_MouseButtonPressed[button];
    }

    bool 
    IsMouseButtonPressedThisFrame(uint8_t button)
    {
        return Input_MouseButtonPressedThisFrame[button];
    }

    bool 
    IsMouseButtonUpThisFrame(uint8_t button)
    {
        return Input_MouseButtonUpThisFrame[button];
    }

    float 
    GetMouseMovementX()
    {
        return Input_MouseMovementX;
    }

    float 
    GetMouseMovementY()
    {
        return Input_MouseMovementY;
    }
}