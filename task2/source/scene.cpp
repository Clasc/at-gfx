struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec3 Binormal;
    glm::vec2 TextureCoordinates;
};

enum MaterialOpacity {
    MaterialOpacityOpaque,
    MaterialOpacityCutout,
    MaterialOpacityTransparent
};

struct Material {
    int AlbedoIndex;
    int MetallicRoughnessIndex;
    int NormalIndex;
    int OcclusionIndex;

    int EmissiveIndex;
    float AlphaCutoff;
    glm::vec2 Padding;

    glm::vec4 AlbedoFactor;

    float RoughnessFactor;
    glm::vec3 EmissiveFactor;
    
    float MetalnessFactor;
    float OcclusionStrength; 
    float NormalScale;
    MaterialOpacity Opacity;

};

struct Group {
    int MaterialIndex;
    uint32_t IndexStart;
    uint32_t IndexCount;
};

struct Mesh {
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
    std::vector<Group> Groups;

    glm::vec3 ExtentsMin;
    glm::vec3 ExtentsMax;

    uint32_t VertexBuffer = 0;
    uint32_t IndexBuffer = 0;
    uint32_t VAO = 0;

    void 
    ComputeExtents() {
        ExtentsMin.x = ExtentsMin.y = ExtentsMin.z = 10000000;
        ExtentsMax.x = ExtentsMax.y = ExtentsMax.z = -10000000;

        size_t vertexCount = Vertices.size();
        for (size_t i = 0; i < vertexCount; ++i) {
            Vertex vert = Vertices[i];
            if(vert.Position.x < ExtentsMin.x) {
                ExtentsMin.x = vert.Position.x;
            }
            if(vert.Position.y < ExtentsMin.y) {
                ExtentsMin.y = vert.Position.y;
            }
            if(vert.Position.z < ExtentsMin.z) {
                ExtentsMin.z = vert.Position.z;
            }
            if(vert.Position.x > ExtentsMax.x) {
                ExtentsMax.x = vert.Position.x;
            }
            if(vert.Position.y > ExtentsMax.y) {
                ExtentsMax.y = vert.Position.y;
            }
            if(vert.Position.z > ExtentsMax.z) {
                ExtentsMax.z = vert.Position.z;
            }
        }
    }

    glm::vec3
    GetExtentCenter() {
        return (ExtentsMin + ExtentsMax) * 0.5f;
    }

    void
    UpdateGPUBuffers() {
        FreeGPUBuffers();

        glGenBuffers(1, &VertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(Vertex), Vertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &IndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(uint32_t), Indices.data(), GL_STATIC_DRAW);

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, Position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, TextureCoordinates));
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
        
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void 
    FreeGPUBuffers() {
        if(VertexBuffer) {
            glDeleteBuffers(1, &VertexBuffer);
            VertexBuffer = 0;
        }
        if(IndexBuffer) {
            glDeleteBuffers(1, &IndexBuffer);
            IndexBuffer = 0;
        }
        if(VAO) {
            glDeleteBuffers(1, &VAO);
            VAO = 0;
        }
    }

    ~Mesh() {
        FreeGPUBuffers();
    }

};

enum LightType {
    LightTypePoint,
    LightTypeSpot,
    LightTypeDirectional
};

struct Node {
    glm::vec3 Position = glm::vec3();
    glm::quat Rotation = glm::identity<glm::quat>();
    glm::vec3 Scale = glm::vec3(1, 1, 1);

    glm::mat4 WorldMatrix = glm::mat4();
    glm::mat4 WorldMatrixInverseTranspose = glm::mat4();

    std::vector<Node*> Children;

    Mesh* LinkedMesh = (Mesh*)0;
};

struct Light {
    Node* ParentNode;
    glm::vec3 Color;
    float Intensity;
    LightType Type;
    float Range;
    float InnerAngle;
    float OuterAngle;
    float Radius;
};

struct Scene {
    tinygltf::Model GLTFModel;
    bool IsValid;

    std::unordered_map<int, Mesh*> Meshes;
    std::vector<Light*> Lights;
    Node* RootNode;

    uint32_t TextureArray = 0;
    uint32_t MaterialBuffer = 0;
    uint32_t MaterialBufferTexture = 0;

    Scene(char* pathToGLTF, bool binaryData = false) {
        RootNode = new Node();
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        if(binaryData) {
            IsValid = loader.LoadBinaryFromFile(&GLTFModel, &err, &warn, pathToGLTF);
        } else {
            IsValid = loader.LoadASCIIFromFile(&GLTFModel, &err, &warn, pathToGLTF);
        }

        if (!warn.empty()) {
            LogWarning("GLTF Loader Warning: %s\n", warn.c_str());
        }

        if (!err.empty()) {
            LogError("GLTF Loader Error: %s\n", err.c_str());
        }

        if(IsValid) {
            loadModel(GLTFModel);
            UpdateNodes();
        }
    }

    Mesh* loadMesh(tinygltf::Model &model, tinygltf::Mesh &mesh) {
        Mesh* newMesh = new Mesh();
        for (size_t m = 0; m < mesh.primitives.size(); ++m) {
            tinygltf::Primitive primitive = mesh.primitives[m];
            tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];
            const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

            const uint16_t* indexArrayShort = 0;
            const uint32_t* indexArrayInteger = 0;
            if(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                indexArrayShort = reinterpret_cast<const uint16_t*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
            } else if(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                indexArrayInteger = reinterpret_cast<const uint32_t*>(&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
            }

            auto attribPosition = primitive.attributes.find("POSITION");
            auto attribNormal = primitive.attributes.find("NORMAL");      
            auto attribTexCoord = primitive.attributes.find("TEXCOORD_0");
            const glm::vec3* positionArray = 0;
            const glm::vec3* normalArray = 0;
            const glm::vec2* texcoordArray = 0;
            size_t vertexCount = 0;

            if(attribPosition != primitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[attribPosition->second];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                positionArray = reinterpret_cast<const glm::vec3*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                vertexCount = accessor.count;
            }

            if(attribNormal != primitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[attribNormal->second];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                normalArray = reinterpret_cast<const glm::vec3*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            if(attribTexCoord != primitive.attributes.end()) {
                const tinygltf::Accessor& accessor = model.accessors[attribTexCoord->second];
                const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                texcoordArray = reinterpret_cast<const glm::vec2*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            }

            uint32_t indexOffset = (uint32_t)newMesh->Vertices.size();
            for (size_t v = 0; v < vertexCount; ++v) { 
                Vertex newVertex;
                newVertex.Position = positionArray[v];
                if(normalArray) {
                    newVertex.Normal = normalArray[v];
                }
                if(texcoordArray) {
                    newVertex.TextureCoordinates = texcoordArray[v];
                }
                newMesh->Vertices.push_back(newVertex);
            }

            Group newGroup;
            newGroup.IndexStart = (uint32_t)newMesh->Indices.size();
            if(indexArrayInteger) {
                for (size_t i = 0; i < indexAccessor.count; ++i) { 
                    newMesh->Indices.push_back(indexOffset + indexArrayInteger[i]);
                }
            } else if(indexArrayShort){
                for (size_t i = 0; i < indexAccessor.count; ++i) { 
                    newMesh->Indices.push_back(indexOffset + indexArrayShort[i]);
                }
            }
            newGroup.IndexCount = (uint32_t)newMesh->Indices.size() - newGroup.IndexStart;
            newGroup.MaterialIndex = primitive.material;

            newMesh->Groups.push_back(newGroup);
        }

        newMesh->UpdateGPUBuffers();

        return newMesh;
    }


    void loadModelNodes(tinygltf::Model &model, tinygltf::Node &node, Node* parentNode) {
       
        Node* newNode = new Node();
        if(node.translation.size() > 0) {
            newNode->Position = glm::vec3((float)(node.translation[0]), (float)(node.translation[1]), (float)(node.translation[2]));
        }
        if(node.rotation.size() > 0) {
            newNode->Rotation = glm::quat((float)(node.rotation[3]), (float)(node.rotation[0]), (float)(node.rotation[1]), (float)(node.rotation[2]));
        }
        if(node.scale.size() > 0) {
            newNode->Scale = glm::vec3((float)(node.scale[0]), (float)(node.scale[1]), (float)(node.scale[2]));
        }
        if(node.mesh != -1) {
            auto foundMesh = Meshes.find(node.mesh);
            if(foundMesh != Meshes.end()) {
                newNode->LinkedMesh = foundMesh->second;
            } else {
                newNode->LinkedMesh = loadMesh(model, model.meshes[node.mesh]);
                Meshes.insert({node.mesh, newNode->LinkedMesh});
            }
        }
        parentNode->Children.push_back(newNode);

        for (size_t i = 0; i < node.children.size(); i++) {
            loadModelNodes(model, model.nodes[node.children[i]], newNode);
        }
    }

    void loadModel(tinygltf::Model &model) {
        if(model.scenes.size() == 0) {
            LogWarning("No scenes found in GLTF file.");
            return;
        }
        int sceneToLoad = model.defaultScene;
        if(sceneToLoad == -1) {
            sceneToLoad = 0;
        }
        const tinygltf::Scene &scene = model.scenes[sceneToLoad];
        for (size_t i = 0; i < scene.nodes.size(); ++i) {
            loadModelNodes(model, model.nodes[scene.nodes[i]], RootNode);
        }

        // Update textures.
        const int32_t TextureSize = 1024;
        int32_t textureCount = (int32_t)model.textures.size();
        int32_t mipLevelCount = (int32_t)glm::floor(log2(TextureSize)) + 1;
        glGenTextures(1, &TextureArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, TextureArray);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, GL_RGBA8, TextureSize, TextureSize, textureCount);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, mipLevelCount - 1);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);

        uint8_t* scaledImage = new uint8_t[TextureSize * TextureSize * 4];
        for(int32_t i = 0; i < textureCount; ++i) {
            tinygltf::Texture &tex = model.textures[i];
            tinygltf::Image &image = model.images[tex.source];

            //Resize all textures to same size and put into texture array.
            stbir_resize_uint8(image.image.data(), image.width, image.height, 0, scaledImage, TextureSize, TextureSize, 0, 4);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, TextureSize, TextureSize, 1, GL_RGBA, GL_UNSIGNED_BYTE, scaledImage);
        }
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        delete[] scaledImage;


        // Update materials.
        size_t materialCount = model.materials.size();
        glGenBuffers(1, &MaterialBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, MaterialBuffer);
        glBufferData(GL_TEXTURE_BUFFER, sizeof(RendererMaterial) * materialCount, 0, GL_DYNAMIC_DRAW);
        RendererMaterial* mappedMaterialBuffer = (RendererMaterial*)glMapBuffer(GL_TEXTURE_BUFFER, GL_WRITE_ONLY);

        for(size_t i = 0; i < materialCount; ++i) {
            tinygltf::Material &mat = model.materials[i];

            mappedMaterialBuffer->AlbedoFactor = glm::vec4((float)mat.pbrMetallicRoughness.baseColorFactor[0], (float)mat.pbrMetallicRoughness.baseColorFactor[1], (float)mat.pbrMetallicRoughness.baseColorFactor[2], (float)mat.pbrMetallicRoughness.baseColorFactor[3]);
            mappedMaterialBuffer->AlphaCutoff = (float)mat.alphaCutoff;
            mappedMaterialBuffer->RoughnessFactor = (float)mat.pbrMetallicRoughness.roughnessFactor;
            mappedMaterialBuffer->EmissiveFactor = glm::vec3((float)mat.emissiveFactor[0], (float)mat.emissiveFactor[1], (float)mat.emissiveFactor[2]);
            mappedMaterialBuffer->MetalnessFactor = (float)mat.pbrMetallicRoughness.metallicFactor;;
            mappedMaterialBuffer->OcclusionStrength = (float)mat.occlusionTexture.strength;
            mappedMaterialBuffer->NormalScale = (float)mat.normalTexture.scale;

            uint32_t featureMask = 0;
            mappedMaterialBuffer->AlbedoMap = mat.pbrMetallicRoughness.baseColorTexture.index;
            if(mappedMaterialBuffer->AlbedoMap != -1) {    
                featureMask |= MATERIAL_FEATURE_ALBEDO_MAP;
            } else {
                featureMask &= ~MATERIAL_FEATURE_ALBEDO_MAP;
            }

            mappedMaterialBuffer->MetallicRoughnessMap = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
            if(mappedMaterialBuffer->MetallicRoughnessMap != -1) {    
                featureMask |= MATERIAL_FEATURE_METALLICROUGHNESS_MAP;
            } else {
                featureMask &= ~MATERIAL_FEATURE_METALLICROUGHNESS_MAP;
            }

            mappedMaterialBuffer->NormalMap = mat.normalTexture.index;
            if(mappedMaterialBuffer->NormalMap != -1) {    
                featureMask |= MATERIAL_FEATURE_NORMAL_MAP;
            } else {
                featureMask &= ~MATERIAL_FEATURE_NORMAL_MAP;
            }

            mappedMaterialBuffer->OcclusionMap = mat.occlusionTexture.index;
            if(mappedMaterialBuffer->OcclusionMap != -1) {    
                featureMask |= MATERIAL_FEATURE_OCCLUSION_MAP;
            } else {
                featureMask &= ~MATERIAL_FEATURE_OCCLUSION_MAP;
            }

            mappedMaterialBuffer->EmissiveMap = mat.emissiveTexture.index;
            if(mappedMaterialBuffer->EmissiveMap != -1) {
                featureMask |= MATERIAL_FEATURE_EMISSIVE_MAP;
            } else {
                featureMask &= ~MATERIAL_FEATURE_EMISSIVE_MAP;
            }

            if(mat.alphaMode.compare("OPAQUE") == 0) {
                featureMask &= ~MATERIAL_FEATURE_OPACITY_CUTOUT;
                featureMask &= ~MATERIAL_FEATURE_OPACITY_TRANSPARENT;
            } else if(mat.alphaMode.compare("MASK") == 0) {
                featureMask |= MATERIAL_FEATURE_OPACITY_CUTOUT;
            } else if(mat.alphaMode.compare("BLEND") == 0) {
                featureMask |= MATERIAL_FEATURE_OPACITY_TRANSPARENT;
            }

            mappedMaterialBuffer->FeatureMask = featureMask;

            ++mappedMaterialBuffer;
        }
        glUnmapBuffer(GL_TEXTURE_BUFFER);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);

        glGenTextures(1, &MaterialBufferTexture);
        glBindTexture(GL_TEXTURE_BUFFER, MaterialBufferTexture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, MaterialBuffer);
        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }

    void DeleteNode(Node* node) {
        for (size_t i = 0; i < node->Children.size(); i++) {
            DeleteNode(node->Children[i]);
        }
        delete node;
    }

    void UpdateNode(Node* node, Node* parent) {
        glm::mat4 scale = glm::scale(node->Scale);
        glm::mat4 rotation = glm::toMat4(node->Rotation);
        glm::mat4 translation = glm::translate(node->Position);
        glm::mat4 localMatrix = translation * rotation * scale;// scale * rotation * translation; // 
        if(parent) {
            node->WorldMatrix = parent->WorldMatrix * localMatrix;
        } else {
            node->WorldMatrix = localMatrix;
        }
        node->WorldMatrixInverseTranspose = glm::transpose(glm::inverse(node->WorldMatrix));

        for (size_t i = 0; i < node->Children.size(); i++) {
            UpdateNode(node->Children[i], node);
        }
    }

    void UpdateNodes() {
        UpdateNode(RootNode, 0);
    }

    ~Scene() {
        DeleteNode(RootNode);
        for(auto mesh : Meshes) {
            delete mesh.second;
        }

        glDeleteBuffers(1, &MaterialBuffer);
        Meshes.clear();
    }
};