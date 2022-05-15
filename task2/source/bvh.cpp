struct BVHBuildTriangle {
    glm::vec3 Min;
    glm::vec3 Max;

    glm::vec3 A;
    glm::vec3 B;
    glm::vec3 C;

    uint32_t TexCoordA;
    uint32_t TexCoordB;
    uint32_t TexCoordC;

    uint32_t MaterialIndex;
};

struct BVHBuildNode {
    glm::vec3 Min;
    glm::vec3 Max;
    std::vector<BVHBuildTriangle>* Triangles;
    std::vector<BVHBuildNode*> Nodes;
    float Cost;

    bool Intersects(glm::vec3 minA, glm::vec3 maxA, glm::vec3 minB, glm::vec3 maxB) {
        if (minA.x > maxB.x || minA.y > maxB.y || minA.z > maxB.z) return false;
        if (minB.x > maxA.x || minB.y > maxA.y || minB.z > maxA.z) return false;
        return true;
    };

    BVHBuildNode() {
        Min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        Max = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        Triangles = new std::vector<BVHBuildTriangle>();
    }

    void ComputeCost() {
        glm::vec3 size = Max - Min;
        Cost = (size.x * size.y * 2 + size.x * size.z * 2 + size.y * size.z * 2) * Triangles->size();
    }

    void Split() {
        if (GetTriangleCount() <= BVH_MAX_TRIANGLES_PER_NODE) {
            return;
        }

        glm::vec3 min = Min;
        glm::vec3 max = Max;
        glm::vec3 size = max - min;

        BVHBuildNode* children[BVH_MAX_CHILD_NODES] = {};
        BVHBuildNode* candidates[BVH_MAX_CHILD_NODES];
        for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
            candidates[c] = new BVHBuildNode();
        }
        float splitAlpha[BVH_MAX_CHILD_NODES];
        float bestCost = FLT_MAX;
        for (int axis = 0; axis < 3; ++axis) {
            const int maxSplits = 10;
            for (int split = 1; split < maxSplits; ++split) {

                #if BVH_USE_RANDOM
                // Compute random splits.
                for (int c = 0; c < BVH_MAX_CHILD_NODES - 1; ++c) {
                    splitAlpha[c] = glm::linearRand(0.0f, 0.99f);
                }

                // Sort splits.
                std::sort(splitAlpha, splitAlpha + (BVH_MAX_CHILD_NODES - 1), std::less<float>());

                #else
                assert(BVH_MAX_CHILD_NODES == 2);
                splitAlpha[0] = (float)split / maxSplits;
                #endif

                // Last split goes to the end.
                splitAlpha[BVH_MAX_CHILD_NODES - 1] = 1.001f;

                float lastSplit = 0;
                for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
                    candidates[c]->Min = min;
                    candidates[c]->Max = max;
                    candidates[c]->Min[axis] = min[axis] + lastSplit * size[axis];
                    lastSplit = splitAlpha[c];
                    candidates[c]->Max[axis] = min[axis] + lastSplit * size[axis];
                    candidates[c]->Triangles->clear();
                }

                for (uint32_t i = 0; i < GetTriangleCount(); ++i) {
                    BVHBuildTriangle triangle = GetTriangle(i);
                    float center = (triangle.Min[axis] + triangle.Max[axis]) * 0.5f;

                    for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
                        if (center >= candidates[c]->Min[axis] && center < candidates[c]->Max[axis]) {
                            candidates[c]->Triangles->push_back(triangle);
                            break;
                        }
                    }
                }

                float costOverall = 0;
                for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
                    candidates[c]->CompactAABB();
                    candidates[c]->ComputeCost();

                    costOverall += candidates[c]->Cost;
                }

                if (costOverall < bestCost) {
                    bestCost = costOverall;
                    for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
                        BVHBuildNode* newCandidate = 0;
                        if (children[c]) {
                            newCandidate = children[c];
                        }
                        if (!newCandidate) {
                            newCandidate = new BVHBuildNode();
                        }
                        children[c] = candidates[c];
                        candidates[c] = newCandidate;
                    }
                }
            }
        }

        // No improvement, randomly split.
        if (bestCost > Cost) {
            for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
                candidates[c]->Triangles->clear();
            }
            int countPerChild = (int)GetTriangleCount() / BVH_MAX_CHILD_NODES;
            for (int i = 0; i < GetTriangleCount(); ++i) {
                BVHBuildTriangle triangle = GetTriangle(i);
                int c = glm::min(i / countPerChild, BVH_MAX_CHILD_NODES - 1);
                candidates[c]->Triangles->push_back(triangle);
            }
            for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
                children[c] = candidates[c];
                children[c]->CompactAABB();
                children[c]->ComputeCost();
                candidates[c] = 0;
            }
        }

        for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
            if (candidates[c]) {
                delete candidates[c];
            }
        }

        // Clear triangle memory.
        delete Triangles;
        Triangles = 0;

        for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
            if (children[c]->Triangles->size() > 0) {
                Nodes.push_back(children[c]);
            }
        }

        for (int c = 0; c < BVH_MAX_CHILD_NODES; ++c) {
            if (children[c]->Triangles && children[c]->Triangles->size() > BVH_MAX_TRIANGLES_PER_NODE) {
                children[c]->Split();
            }
        }
    }

    BVHBuildTriangle GetTriangle(size_t index) {
        if (Triangles) {
            return Triangles->at(index);
        }
        return BVHBuildTriangle();
    }

    size_t GetTriangleCount() {
        if (Triangles) {
            return Triangles->size();
        }
        return 0;
    }

    void CompactAABB() {
        Min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
        Max = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        if (Triangles) {
            for (uint32_t i = 0; i < Triangles->size(); ++i) {
                Min = glm::min(Min, Triangles->at(i).Min);
                Max = glm::max(Max, Triangles->at(i).Max);
            }
        }
    }

    void Draw(int depth = 0, int maxDepth = 16) {
        Debug::DrawBoxMinMax(Min, Max, Debug::RGBAColor(1.0f, 0.0f, 0.0f, 1.0f));
        if (maxDepth == depth) {
            return;
        }
        for (int i = 0; i < Nodes.size(); ++i) {
            Nodes[i]->Draw(depth + 1, maxDepth);
        }
    }

    ~BVHBuildNode() {
        for (int i = 0; i < Nodes.size(); ++i) {
            delete Nodes[i];
        }
        delete Triangles;
    }

    uint32_t GetNodeCount() {
        uint32_t nodeCount = 1;
        for (int i = 0; i < Nodes.size(); ++i) {
            nodeCount += Nodes[i]->GetNodeCount();
        }
        return nodeCount;
    }


};

struct IterativeNode {
    size_t childrenStart;
    size_t childCount;
    size_t triangleCount;
    size_t triangleStart;
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;

    IterativeNode(const BVHBuildNode* bvhNode, size_t childrenStart, size_t triangleStart) {
        childCount = bvhNode->Nodes.size();
        this->childrenStart = childCount == 0 ? 0 : childrenStart;

        triangleCount = bvhNode->Triangles ? bvhNode->Triangles->size() : 0;
        this->triangleStart = triangleCount == 0 ? 0 : triangleStart;

        aabbMin = bvhNode->Min;
        aabbMax = bvhNode->Max;
    }
};

struct IterativeBVH {
    std::vector<IterativeNode> nodes;
    std::vector<BVHBuildTriangle*> triangles;

    IterativeBVH() {
        nodes = std::vector<IterativeNode>();
        triangles = std::vector<BVHBuildTriangle*>();
    }

    void convertBvhToIterative(const BVHBuildNode* bvhNode) {
        auto triangleOffset = triangles.size();
        if (bvhNode == nullptr) {
            return;
        }

        if (bvhNode->Triangles) {
            for (uint32_t i = 0; i < bvhNode->Triangles->size(); ++i) {
                triangles.push_back(&bvhNode->Triangles->at(i));
            }
        }

        auto childrenStart = nodes.size() + 1;
        nodes.push_back(IterativeNode(bvhNode, childrenStart, triangleOffset));
        for (auto node : bvhNode->Nodes) {
            convertBvhToIterative(node);
        }
    }
};

struct BVH {
    BVHBuildNode* BVHRoot;
    uint32_t SceneTriangleCount;
    uint32_t BVHNodeCount;

    BVH() {}

    void AddTrianglesToRoot(Node* node) {
        if (node->LinkedMesh) {
            Mesh* mesh = node->LinkedMesh;
            glm::mat4 transform = node->WorldMatrix;

            for (int g = 0; g < mesh->Groups.size(); ++g) {
                Group group = mesh->Groups[g];
                for (uint32_t i = group.IndexStart; i < group.IndexStart + group.IndexCount; i += 3) {
                    uint32_t i0 = mesh->Indices[i + 0];
                    uint32_t i1 = mesh->Indices[i + 1];
                    uint32_t i2 = mesh->Indices[i + 2];

                    Vertex v0 = mesh->Vertices[i0];
                    Vertex v1 = mesh->Vertices[i1];
                    Vertex v2 = mesh->Vertices[i2];

                    BVHBuildTriangle bvhTriangle;
                    bvhTriangle.MaterialIndex = group.MaterialIndex;
                    buildTriangle(&bvhTriangle, transform, v0, v1, v2);
                    BVHRoot->Min = glm::min(BVHRoot->Min, bvhTriangle.Min);
                    BVHRoot->Max = glm::max(BVHRoot->Max, bvhTriangle.Max);
                    BVHRoot->Triangles->push_back(bvhTriangle);
                }
            }
        }
        for (size_t i = 0; i < node->Children.size(); i++) {
            AddTrianglesToRoot(node->Children[i]);
        }
    }

    void buildTriangle(BVHBuildTriangle* bvhTriangle, glm::mat4 transform, Vertex v0, Vertex v1, Vertex v2) {
        bvhTriangle->A = glm::vec3(transform * glm::vec4(v0.Position, 1));
        bvhTriangle->B = glm::vec3(transform * glm::vec4(v1.Position, 1));
        bvhTriangle->C = glm::vec3(transform * glm::vec4(v2.Position, 1));

        bvhTriangle->TexCoordA = glm::packHalf2x16(v0.TextureCoordinates);
        bvhTriangle->TexCoordB = glm::packHalf2x16(v1.TextureCoordinates);
        bvhTriangle->TexCoordC = glm::packHalf2x16(v2.TextureCoordinates);

        bvhTriangle->Min = bvhTriangle->A;
        bvhTriangle->Max = bvhTriangle->A;
        bvhTriangle->Min = glm::min(bvhTriangle->Min, bvhTriangle->B);
        bvhTriangle->Min = glm::min(bvhTriangle->Min, bvhTriangle->C);
        bvhTriangle->Max = glm::max(bvhTriangle->Max, bvhTriangle->B);
        bvhTriangle->Max = glm::max(bvhTriangle->Max, bvhTriangle->C);
    }

    void GenerateBVH(Scene* scene) {
        SceneTriangleCount = 0;
        BVHRoot = new BVHBuildNode();

        AddTrianglesToRoot(scene->RootNode);

        QueryCPU* querySplit = GlobalProfiler.StartCPUQuery("Renderer::Build BVH (Split)");
        BVHRoot->ComputeCost();
        BVHRoot->Split();
        BVHNodeCount = BVHRoot->GetNodeCount();
        GlobalProfiler.StopCPUQuery(querySplit);

        auto iterativeBvh = IterativeBVH();
        iterativeBvh.convertBvhToIterative(BVHRoot);
    }

    void Draw(int maxNodeLevel) {
        if (BVHRoot) {
            BVHRoot->Draw(0, maxNodeLevel);
        }
    }
};