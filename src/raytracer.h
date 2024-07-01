#pragma once
#include "util.h"
#include <iostream>



struct BVHNode {
    BoundingBox bb;
    // index of the triangles inside this node
    std::vector<uint32_t> triangle_idx;
    BVHNode* child_1 = nullptr;
    BVHNode* child_2 = nullptr;
};

struct BoundingVolumeHierarchy {
    struct Triangle {
        Vertex v1;
        Vertex v2;
        Vertex v3;
        glm::vec3 center;
    };
    BoundingVolumeHierarchy() {};
    BoundingVolumeHierarchy(const BoundingVolumeHierarchy&) = delete;
    BoundingVolumeHierarchy(BoundingVolumeHierarchy&&);
    BoundingVolumeHierarchy& operator=(BoundingVolumeHierarchy&&);

    void CreateFromMesh(const Mesh* mesh, uint32_t max_steps);
    void Destroy();
    std::vector<Triangle> triangle_data;
    BVHNode root_node;
};

struct RayMaterial {
    glm::vec4 emission_col;
    glm::vec4 specular_col;
    float emission_strength;
    float smoothness;
    float specular_probability;
};

struct RayObject {
    BoundingVolumeHierarchy* bvh;
    glm::mat4 mat;
    glm::mat4 inv_model_mat;
    RayMaterial material;
};

struct RayScene {
    std::vector<RayObject> objects;
};



struct RayCamera {
    glm::vec3 pos;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 forward;
    glm::vec3 bottom_left_local;

    float plane_width;
    float plane_height;

    void SetFromMatrix(const glm::mat4& mat, float fov, float near_plane, float aspect_ratio) {
        const glm::mat4 inv_view_mat = glm::inverse(mat);
        this->right     = -glm::vec3(inv_view_mat[0][0], inv_view_mat[0][1], inv_view_mat[0][2]);
        this->up        = glm::vec3(inv_view_mat[1][0], inv_view_mat[1][1], inv_view_mat[1][2]);
        this->forward   = -glm::vec3(inv_view_mat[2][0], inv_view_mat[2][1], inv_view_mat[2][2]);
        this->pos       = glm::vec3(inv_view_mat[3][0], inv_view_mat[3][1], inv_view_mat[3][2]);

        this->plane_height = near_plane * glm::tan(fov * 0.5f) * 2.0f;
        this->plane_width = this->plane_height * aspect_ratio;
        this->bottom_left_local = glm::vec3(-this->plane_width / 2.0f, -this->plane_height / 2.0f, near_plane);
    }
};


struct RayImage {
    RayImage(uint32_t w, uint32_t h) {
        this->colors = new glm::vec4[w * h];
        this->width = w;
        this->height = h;
        this->num_rendered_frames = 0;
    }
    RayImage() { this->colors = nullptr; }
    RayImage(const RayImage&) = delete; 
    RayImage(RayImage&&);
    RayImage& operator=(RayImage&&);


    void Destroy();
    void Clear() {
        if(!colors) return;
        for(uint32_t y = 0; y < this->height; ++y) {
            for(uint32_t x = 0; x < this->width; ++x) {
                this->colors[y * this->width + x] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            }
        }
        this->num_rendered_frames = 0;
    }
    glm::vec4* colors;
    uint32_t width;
    uint32_t height;
    uint32_t num_rendered_frames;
};

RayImage RayTraceMesh(const RayCamera& cam, const Mesh& mesh, const glm::mat4& inv_model_mat, uint32_t w, uint32_t h);
RayImage RayTraceBVH(const RayCamera& cam, const BoundingVolumeHierarchy& bvh, const glm::mat4& inv_model_mat, uint32_t w, uint32_t h);
RayImage RayTraceScene(const RayCamera& cam, const RayScene& scene, uint32_t max_bounces, uint32_t sample_count, uint32_t w, uint32_t h);
void RayTraceSceneAccumulate(RayImage& img, const RayCamera& cam, const RayScene& scene, uint32_t max_bounces, uint32_t sample_count);



