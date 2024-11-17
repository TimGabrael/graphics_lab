#pragma once
#include "util.h"

struct FastDynamicMesh {
    Mesh* mesh;
    struct TriangleData {
        uint32_t vtx_1;
        uint32_t vtx_2;
        uint32_t vtx_3;
        uint32_t neighbour_12 = SIZE_MAX;
        uint32_t neighbour_13 = SIZE_MAX;
        uint32_t neighbour_23 = SIZE_MAX;
        float area;
    };
    struct EdgeData {
        uint32_t vtx_1;
        uint32_t vtx_2;
        float len;
    };
    struct VertexData {
        std::vector<uint32_t> triangles;
        std::vector<uint32_t> edges;
    };
    std::vector<TriangleData> triangles;
    std::vector<EdgeData> edges;
    std::vector<VertexData> vertex_data;
    void Initialize(Mesh* underlying_mesh);
};

glm::vec2 GetMeshMinMaxTrianglesSizes(const Mesh* input);

Mesh DecimateMesh(const Mesh* input, float min_triangle_size);



