
struct Camera
{
    glm::vec3 Position;
    glm::vec3 Direction;
    float FieldOfView;
    float Nearplane;
    float Farplane;
    float AspectRatio;
    float Exposure = 0;
    
    glm::mat4x4 View;
    glm::mat4x4 ViewInv;
    glm::mat4x4 Projection;
    glm::mat4x4 ProjectionInv;
    glm::mat4x4 ViewProjection;
    glm::mat4x4 ViewProjectionOld;
    glm::mat4x4 ViewProjectionInv;
    glm::mat4x4 ViewProjectionUnjittered;

    glm::vec2 JitterOld = glm::vec2();
    glm::vec2 Jitter = glm::vec2();
    glm::vec2 JitterSamples[RENDERING_TAA_SAMPLE_COUNT];
    int CurrentJitterSample = 0;

    Camera()
    {
        View = glm::mat4x4();
        Projection = glm::mat4x4();
        ViewProjection = glm::mat4x4();
        ViewProjectionInv = glm::mat4x4();

        Halton_sampler hs;
        hs.init_faure();
        for (int i = 0; i < ArrayCount(JitterSamples); ++i) {
            JitterSamples[i] = glm::vec2(hs.sample(0, i), hs.sample(1, i));
        }
    }

    void UpdateProjectionMatrix()
    {
        float yScale = glm::cot(FieldOfView / 2.0f);
        float xScale = yScale / AspectRatio;

        Projection[0][0] = xScale;
        Projection[0][1] = 0;
        Projection[0][2] = 0;
        Projection[0][3] = 0;

        Projection[1][0] = 0;
        Projection[1][1] = yScale;
        Projection[1][2] = 0;
        Projection[1][3] = 0;

        Projection[2][0] = 0;
        Projection[2][1] = 0;
        Projection[2][2] = Farplane / (Farplane - Nearplane);
        Projection[2][3] = -Nearplane * Farplane / (Farplane - Nearplane);

        Projection[3][0] = 0;
        Projection[3][1] = 0;
        Projection[3][2] = 1;
        Projection[3][3] = 0;

        ViewProjectionUnjittered = View * Projection;
        Projection[0][2] = Jitter.x;      
        Projection[1][2] = Jitter.y;    
        ProjectionInv = glm::inverse(Projection);  
        ViewProjection =  View * Projection;
        ViewProjectionInv = glm::inverse(ViewProjection);  
    }

    void UpdateViewMatrix()
    {
        glm::vec3 up = GetUp();
        glm::vec3 at = Position + Direction;

        glm::vec3 zAxis = glm::normalize(at - Position);
        glm::vec3 xAxis = glm::normalize(glm::cross(up, zAxis));
        glm::vec3 yAxis = glm::cross(zAxis, xAxis);

        View[0][0] = xAxis.x;
        View[0][1] = xAxis.y;
        View[0][2] = xAxis.z;
        View[0][3] = -glm::dot(xAxis, Position);

        View[1][0] = yAxis.x;
        View[1][1] = yAxis.y;
        View[1][2] = yAxis.z;
        View[1][3] = -glm::dot(yAxis, Position);

        View[2][0] = zAxis.x;
        View[2][1] = zAxis.y;
        View[2][2] = zAxis.z;
        View[2][3] = -glm::dot(zAxis, Position);

        View[3][0] = 0;
        View[3][1] = 0;
        View[3][2] = 0;
        View[3][3] = 1;

        ViewInv = glm::inverse(View);
        ViewProjection = View * Projection;
        ViewProjectionInv = glm::inverse(ViewProjection);
    }

    void NewFrame(int width, int height, bool taa) {
        ViewProjectionOld = ViewProjection;
        
        if(taa) {
            ++CurrentJitterSample;
            if(CurrentJitterSample >= ArrayCount(JitterSamples)) {
                CurrentJitterSample = 0;
            }
            JitterOld = Jitter;
            Jitter = glm::vec2((2.0f * JitterSamples[CurrentJitterSample].x - 1.0f) / (float)(width), (2.0f * JitterSamples[CurrentJitterSample].y - 1.0f) / (float)(height));
        } else {
            Jitter = glm::vec2();   
        }
        UpdateProjectionMatrix();
    }

    void MoveForward(float speed)        
    {
        Position += Direction * speed;
        UpdateViewMatrix();
    }

    void MoveLeft(float speed)
    {
        glm::vec3 dir = glm::cross(Direction, glm::vec3(0, 1, 0));
        Position += dir * speed;
        UpdateViewMatrix();
    }

    void RotateY(float angle)
    {
        Direction = glm::rotate(Direction, angle, glm::vec3(0, 1, 0));
        Direction = glm::normalize(Direction);
        UpdateViewMatrix();
    }

    void RotateX(float angle)
    {
        glm::vec3 dir = glm::cross(Direction, glm::vec3(0, 1, 0));
        Direction = glm::rotate(Direction, -angle, dir);
        Direction = glm::normalize(Direction);
        UpdateViewMatrix();
    }

    void LookAt(glm::vec3 target)
    {
        Direction = target - Position;
        Direction = glm::normalize(Direction);
        UpdateViewMatrix();
    }

    glm::vec3 GetUp() {
        glm::vec3 up = glm::vec3(0, 1, 0);
        if(glm::abs(Direction.y) > 0.999f) {
            up = glm::vec3(1, 0, 0);
        }

        glm::vec3 right = glm::normalize(glm::cross(Direction, up));
        
        return glm::normalize(glm::cross(right, Direction));
    }

    glm::vec3 GetRight() {
        glm::vec3 up = glm::vec3(0, 1, 0);
        if(glm::abs(Direction.y) > 0.999f) {
            up = glm::vec3(1, 0, 0);
        }

        return glm::normalize(glm::cross(Direction, up));
    }
};