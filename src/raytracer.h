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


struct RayImage {
    RayImage(uint32_t w, uint32_t h) {
        this->colors = new glm::vec4[w * h];
        this->width = w;
        this->height = h;
        this->num_rendered_frames = 0;
        this->frame_scale = 1.0f;
        this->FullyClear();
    }
    ~RayImage() {
        this->Destroy();
    }
    RayImage() { this->colors = nullptr; this->width = 0; this->height = 0; this->num_rendered_frames = 0; this->frame_scale = 1.0f; }
    RayImage(const RayImage&) = delete; 
    RayImage(RayImage&& o) {
        this->colors = o.colors;
        this->width = o.width;
        this->height = o.height;
        this->num_rendered_frames = o.num_rendered_frames;
        o.colors = nullptr;
    }
    RayImage& operator=(RayImage&& o) {
        this->Destroy();
        this->colors = o.colors;
        this->width = o.width;
        this->height = o.height;
        this->num_rendered_frames = o.num_rendered_frames;
        o.colors = nullptr;
        return *this;
    }

    void AddColor(const glm::vec4& col, float uv_x, float uv_y) {
        const uint32_t ix = (uint32_t)this->width * uv_x;
        const uint32_t iy = (uint32_t)this->height * uv_y;
        if(ix < this->width && iy < this->height) {
            this->colors[iy * this->width + ix] = this->colors[iy * this->width + ix] * (1.0f - this->frame_scale) + col * this->frame_scale;
        }
    }


    void PostProcessLightMap(RayImage& lightmap) {
        for(int i = 0; i < 16; ++i) {
            this->FullyDilate(lightmap);
        }
        this->Smooth(lightmap, lightmap.width, lightmap.height);
        lightmap.Dilate(*this, lightmap.width, lightmap.height);
    }
    void FullyDilate(RayImage& image) {
        this->Dilate(image, image.width, image.height);
        image.Dilate(*this, image.width, image.height);
    }
    // dialates the input image and stores it in the current image
    // the width and height need to be specified, as this function is a postprocessing step
    // and needs to be done in two directions, but i probably want to keep a larger temporary
    // image that stays in memory and gets used for multiple images meaning it needs to be able 
    // to be addressed as a smaller image in the second pass
    void Dilate(const RayImage& image, uint32_t w, uint32_t h) {
        assert(w <= image.width && h <= image.height);
        assert(w <= this->width && h <= this->height);
        for(uint32_t y = 0; y < h; ++y) {
            for(uint32_t x = 0; x < w; ++x) {
                glm::vec4 col = image.colors[y * w + x];
                if(col.x <= 0.0f && col.y <= 0.0f && col.z <= 0.0f && col.w <= 0.0f) {
                    int n = 0;
                    const int dx[] = {-1, 0, 1,  0 };
                    const int dy[] = { 0, 1, 0, -1 };
                    for(int d = 0; d < 4; ++d) {
                        const uint32_t cx = x + dx[d];
                        const uint32_t cy = y + dy[d];
                        if(cx < w && cy < h) {
                            glm::vec4 dcol = image.colors[cy * image.width + cx];
                            if(dcol.x > 0.0f || dcol.y > 0.0f || dcol.z > 0.0f || dcol.w > 0.0f) {
                                col += dcol;
                                n++;
                            }
                        }
                    }
                    if(n) {
                        const float inverse = 1.0f / (float)n;
                        col *= inverse;
                    }
                }
                this->colors[y * w + x] = col;
            }
        }
    }
    void Smooth(const RayImage& image, uint32_t w, uint32_t h) {
        assert(w <= image.width && h <= image.height);
        assert(w <= this->width && h <= this->height);
        for(uint32_t y = 0; y < h; ++y) {
            for(uint32_t x = 0; x < w; ++x) {
                glm::vec4 col = {};
                int n = 0;
                for(int dy = -1; dy <= 1; ++dy) {
                    uint32_t cy = y + dy;
                    if(cy < h) {
                        for(int dx = -1; dx <= 1; ++dx) {
                            uint32_t cx = x + dx;
                            const glm::vec4& cur_col = image.colors[cy * w + cx];
                            if(cx < w && (cur_col.x > 0.0f || cur_col.y > 0.0f || cur_col.z > 0.0f || cur_col.a > 0.0f)) {
                                col += cur_col;
                                n++;
                            }
                        }
                    }
                }
                if(n) {
                    const float inverse = 1.0f / (float)n;
                    this->colors[y * w + x] = col * inverse;
                }
                else {
                    this->colors[y * w + x] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                }
            }
        }

    }


    void Destroy() {
        if(this->colors) {
            delete[] this->colors;
        }
        this->colors = nullptr;
    }
    void FullyClear() {
        if(!this->colors) return;
        for(uint32_t y = 0; y < this->height; ++y) {
            for(uint32_t x = 0; x < this->width; ++x) {
                this->colors[y * this->width + x] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
        this->num_rendered_frames = 0;
    }
    // clear with alpha = 1.0
    void Clear() {
        if(!this->colors) return;
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
    float frame_scale;
};


struct RayObject {
    BoundingVolumeHierarchy* bvh;
    glm::mat4 mat;
    glm::mat4 inv_model_mat;
    RayMaterial material;
};
struct LitObject {
    RayImage lightmap;
    uint32_t scene_idx;
};

struct RayScene {
    const Texture2D* hdr_map;
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



RayImage RayTraceMesh(const RayCamera& cam, const Mesh& mesh, const glm::mat4& inv_model_mat, uint32_t w, uint32_t h);
RayImage RayTraceBVH(const RayCamera& cam, const BoundingVolumeHierarchy& bvh, const glm::mat4& inv_model_mat, uint32_t w, uint32_t h);
RayImage RayTraceScene(const RayCamera& cam, const RayScene& scene, uint32_t max_bounces, uint32_t sample_count, uint32_t w, uint32_t h);
void RayTraceSceneAccumulate(RayImage& img, const RayCamera& cam, const RayScene& scene, uint32_t max_bounces, uint32_t sample_count);

void RayTraceMapper(LitObject& lit, const RayScene& scene, uint32_t max_bounces, uint32_t sample_count);


