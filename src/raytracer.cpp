#include "raytracer.h"
#include "helper_math.h"
#include <iostream>

struct RayHitResult {
    const RayMaterial* material;
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 col;
    glm::vec2 uv;
    float length;
    bool hit;
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;
};

static bool PosInBoundingBox(const BoundingBox& bb, const glm::vec3& pos) {
    if(bb.min.x <= pos.x && bb.min.y <= pos.y && bb.min.z <= pos.z && pos.x <= bb.max.x && pos.y <= bb.max.y && pos.z <= bb.max.z) {
        return true;
    }
    return false;
}

static RayHitResult RayTriangleIntersection(const Ray& ray, const Vertex& v1, const Vertex& v2, const Vertex& v3) {
    const glm::vec3 v21 = v2.pos - v1.pos;
    const glm::vec3 v31 = v3.pos - v1.pos;
    const glm::vec3 normal = glm::cross(v21, v31);
    const glm::vec3 ao = ray.origin - v1.pos;
    const glm::vec3 dao = glm::cross(ao, ray.dir);

    const float determinant = -glm::dot(ray.dir, normal);
    const float inv_det = 1.0f / determinant;

    const float dst = glm::dot(ao, normal) * inv_det;
    const float u = glm::dot(v31, dao) * inv_det;
    const float v = -glm::dot(v21, dao) * inv_det;
    const float w = 1.0f - u - v;


    RayHitResult hit_result = {};
    hit_result.hit = determinant >= 1e-8 && dst >= 0.0f && u >= 0.0f && v >= 0.0f && w >= 0.0f;
    hit_result.pos = ray.origin + ray.dir * dst;
    hit_result.normal = glm::normalize(v1.nor * w + v2.nor * u + v3.nor * v);
    hit_result.col = (v1.col * w + v2.col * u + v3.col * v);
    hit_result.uv = (v1.uv * w + v2.uv * u + v3.uv * v);
    hit_result.length = dst;

    return hit_result;
}
static RayHitResult RayTriangleIntersection(const Ray& ray, const BoundingVolumeHierarchy::Triangle& triangle) {
    return RayTriangleIntersection(ray, triangle.v1, triangle.v2, triangle.v3);
}
// returns the distance to the bounding box
// if the bounding box is missed, it returns INFINITY
static float RayBoundingBoxIntersectionTest(const Ray& ray, const BoundingBox& bb) {
    glm::vec3 inv = 1.0f / ray.dir;
    const glm::vec3 min_t = (bb.min - ray.origin) * inv;
    const glm::vec3 max_t = (bb.max - ray.origin) * inv;

    const float tmin = glm::max(glm::max(glm::min(min_t.x, max_t.x), glm::min(min_t.y, max_t.y)), glm::min(min_t.z, max_t.z));
    const float tmax = glm::min(glm::min(glm::max(min_t.x, max_t.x), glm::max(min_t.y, max_t.y)), glm::max(min_t.z, max_t.z));

    // box is behind the ray
    if (tmax < 0.0f) {
        return INFINITY;
    }
    // no intersection
    if (tmin > tmax) {
        return INFINITY;
    }
    return tmin;
}

static void NodeCalculateNext(const std::vector<BoundingVolumeHierarchy::Triangle>& triangles, BVHNode* cur_node, uint32_t cur_depth, uint32_t max_depth) {
    if(cur_depth > max_depth || cur_node->triangle_idx.size() < 6) {
        return;
    }
    glm::vec3 bb_sz = cur_node->bb.max - cur_node->bb.min;
    BVHNode* child_1 = new BVHNode;
    BVHNode* child_2 = new BVHNode;
    child_1->child_1 = nullptr;
    child_1->child_2 = nullptr;
    child_2->child_1 = nullptr;
    child_2->child_2 = nullptr;
    child_1->bb = cur_node->bb;
    child_2->bb = cur_node->bb;
    if(bb_sz.x > bb_sz.y) {
        if(bb_sz.x > bb_sz.z) {
            const float new_x = cur_node->bb.min.x + bb_sz.x / 2.0f;
            child_1->bb.max.x = new_x;
            child_2->bb.min.x = new_x;
        }
        else {
            const float new_z = cur_node->bb.min.z + bb_sz.z / 2.0f;
            child_1->bb.max.z = new_z;
            child_2->bb.min.z = new_z;
        }
    }
    else {
        if(bb_sz.z > bb_sz.y) {
            const float new_z = cur_node->bb.min.z + bb_sz.z / 2.0f;
            child_1->bb.max.z = new_z;
            child_2->bb.min.z = new_z;
        }
        else {
            const float new_y = cur_node->bb.min.y + bb_sz.y / 2.0f;
            child_1->bb.max.y = new_y;
            child_2->bb.min.y = new_y;
        }
    }
    for(uint32_t v : cur_node->triangle_idx) {
        auto& trig = triangles.at(v);
        const glm::vec3 bb1_center = (child_1->bb.min + child_1->bb.max) / 2.0f;
        const glm::vec3 bb2_center = (child_2->bb.min + child_2->bb.max) / 2.0f;
        const float len_1 = glm::length(trig.center - bb1_center);
        const float len_2 = glm::length(trig.center - bb2_center);
        if(len_1 < len_2) {
            child_1->triangle_idx.push_back(v);
        }
        else {
            child_2->triangle_idx.push_back(v);
        }
    }
    // expand the bounding boxes to include all triangles fully
    for(uint32_t v : child_1->triangle_idx) {
        auto& trig = triangles.at(v);
        child_1->bb.max = glm::max(trig.v3.pos, glm::max(trig.v2.pos, glm::max(trig.v1.pos, child_1->bb.max)));
        child_1->bb.min = glm::min(trig.v3.pos, glm::min(trig.v2.pos, glm::min(trig.v1.pos, child_1->bb.min)));
    }
    for(uint32_t v : child_2->triangle_idx) {
        auto& trig = triangles.at(v);
        child_2->bb.max = glm::max(trig.v3.pos, glm::max(trig.v2.pos, glm::max(trig.v1.pos, child_2->bb.max)));
        child_2->bb.min = glm::min(trig.v3.pos, glm::min(trig.v2.pos, glm::min(trig.v1.pos, child_2->bb.min)));
    }
    if(child_1->triangle_idx.empty() || child_2->triangle_idx.empty()) {
        // the non empty child is exactly the same as the cur_node
        // so don't bother adding them
        delete child_1;
        delete child_2;
    }
    else {
        cur_node->child_1 = child_1;
        cur_node->child_2 = child_2;
        NodeCalculateNext(triangles, cur_node->child_1, cur_depth + 1, max_depth);
        NodeCalculateNext(triangles, cur_node->child_2, cur_depth + 1, max_depth);
    }
}

void BoundingVolumeHierarchy::CreateFromMesh(const Mesh* mesh, uint32_t max_steps) {
    this->root_node.bb = mesh->bb;
    this->triangle_data.reserve(mesh->triangle_count);
    this->root_node.triangle_idx.reserve(mesh->triangle_count);
    for(size_t i = 0; i < mesh->triangle_count; ++i) {
        uint32_t idx_1 = 3 * i + 0;
        uint32_t idx_2 = 3 * i + 1;
        uint32_t idx_3 = 3 * i + 2;
        if(mesh->inds) {
            idx_1 = mesh->inds[idx_1];
            idx_2 = mesh->inds[idx_2];
            idx_3 = mesh->inds[idx_3];
        }

        auto& v1 = mesh->verts[idx_1];
        auto& v2 = mesh->verts[idx_2];
        auto& v3 = mesh->verts[idx_3];
        Triangle cur_triangle = {};
        cur_triangle.v1 = v1;
        cur_triangle.v2 = v2;
        cur_triangle.v3 = v3;
        cur_triangle.center = (v1.pos + v2.pos + v3.pos) / 3.0f;
        this->triangle_data.push_back(cur_triangle);
        this->root_node.triangle_idx.push_back(i);
    }
    NodeCalculateNext(this->triangle_data, &this->root_node, 0, max_steps);
}
static void DestoryBoundingVolumeHierarchyNode(BVHNode* node) {
    if(node->child_1) {
        DestoryBoundingVolumeHierarchyNode(node->child_1);
    }
    if(node->child_2) {
        DestoryBoundingVolumeHierarchyNode(node->child_2);
    }
    delete node;
}
BoundingVolumeHierarchy::BoundingVolumeHierarchy(BoundingVolumeHierarchy&& o) {
    this->root_node = o.root_node;
    this->triangle_data = std::move(o.triangle_data);
}
BoundingVolumeHierarchy& BoundingVolumeHierarchy::operator=(BoundingVolumeHierarchy&& o) {
    this->Destroy();
    this->root_node = o.root_node;
    this->triangle_data = std::move(o.triangle_data);
    return *this;
}
void BoundingVolumeHierarchy::Destroy() {
    if(this->root_node.child_1) {
        DestoryBoundingVolumeHierarchyNode(this->root_node.child_1);
        delete this->root_node.child_1;
        this->root_node.child_1 = nullptr;
    }
    if(this->root_node.child_2) {
        DestoryBoundingVolumeHierarchyNode(this->root_node.child_2);
        delete this->root_node.child_2;
        this->root_node.child_2 = nullptr;
    }
}
static RayHitResult RayBoundingVolumeHierarchyTest(const Ray& ray, const BoundingVolumeHierarchy& bvh) {
    static std::vector<const BVHNode*> node_stack;
    RayHitResult hit_result = {};
    hit_result.hit = false;
    hit_result.length = INFINITY;
    if(RayBoundingBoxIntersectionTest(ray, bvh.root_node.bb) != INFINITY) {
        node_stack.push_back(&bvh.root_node);
        while(node_stack.size() > 0){
            const BVHNode* cur_node = node_stack.at(node_stack.size() - 1);
            node_stack.erase(node_stack.end() - 1);
            if(cur_node->child_1 && cur_node->child_2) {
                const float dist1 = RayBoundingBoxIntersectionTest(ray, cur_node->child_1->bb);
                const float dist2 = RayBoundingBoxIntersectionTest(ray, cur_node->child_2->bb);
                if(dist1 == INFINITY && dist2 == INFINITY) {
                    continue;
                }
                else if(dist1 == INFINITY) {
                    node_stack.push_back(cur_node->child_2);
                }
                else if(dist2 == INFINITY) {
                    node_stack.push_back(cur_node->child_1);
                }
                else if(dist1 > dist2) {
                    node_stack.push_back(cur_node->child_2);
                    node_stack.push_back(cur_node->child_1);
                }
                else {
                    node_stack.push_back(cur_node->child_1);
                    node_stack.push_back(cur_node->child_2);
                }
            }
            else {
                for(uint32_t trig_idx : cur_node->triangle_idx) {
                    const BoundingVolumeHierarchy::Triangle& trig = bvh.triangle_data.at(trig_idx);
                    RayHitResult trig_hit_result = RayTriangleIntersection(ray, trig);
                    if(trig_hit_result.hit && trig_hit_result.length < hit_result.length) {
                        hit_result = trig_hit_result;
                    }
                }
                if(hit_result.hit) {
                    node_stack.clear();
                    return hit_result;
                }
            }
        }
    }
    node_stack.clear();
    return hit_result;
}

RayImage RayTraceMesh(const RayCamera& cam, const Mesh& mesh, const glm::mat4& inv_model_mat, uint32_t w, uint32_t h) {
    RayImage image(w, h);
    for(size_t i = 0; i < image.width * image.height; ++i) {
        image.colors[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    const float sx = 1.0f / (float)w;
    const float sy = 1.0f / (float)h;
    for(uint32_t j = 0; j < h; ++j) {
        const float py = sy * (h - 1 - j);
        for(uint32_t i = 0; i < w; ++i) {
            const float px = sx * (w - 1 - i);
            const glm::vec3 point_local = cam.bottom_left_local + glm::vec3(cam.plane_width * px, cam.plane_height * py, 0.0f);
            const glm::vec3 point = cam.pos + cam.right * point_local.x + cam.up * point_local.y + cam.forward * point_local.z;


            Ray ray = {};
            ray.dir = glm::normalize(point - cam.pos);
            ray.origin = inv_model_mat * glm::vec4(glm::vec3(cam.pos.x, cam.pos.y, cam.pos.z), 1.0f);
            ray.dir = inv_model_mat * glm::vec4(glm::vec3(ray.dir.x, ray.dir.y, ray.dir.z), 0.0f);

            if(RayBoundingBoxIntersectionTest(ray, mesh.bb) == INFINITY) {
                continue;
            }
            float closest_hit = INFINITY;
            for(uint32_t k = 0; k < mesh.triangle_count; ++k) {
                uint32_t idx_1 = 3 * k + 0;
                uint32_t idx_2 = 3 * k + 1;
                uint32_t idx_3 = 3 * k + 2;
                if(mesh.inds) {
                    idx_1 = mesh.inds[idx_1];
                    idx_2 = mesh.inds[idx_2];
                    idx_3 = mesh.inds[idx_3];
                }

                const Vertex& v1 = mesh.verts[idx_1];
                const Vertex& v2 = mesh.verts[idx_2];
                const Vertex& v3 = mesh.verts[idx_3];
                RayHitResult hit_result = RayTriangleIntersection(ray, v1, v2, v3);

                if(hit_result.hit && hit_result.length < closest_hit) {
                    image.colors[j * w + i] = hit_result.col;
                    closest_hit = hit_result.length;
                }
            }
            if(closest_hit == INFINITY) {
                //image.colors[j * w + i] = {1.0f, 1.0f / (float)w * (float)i, 1.0f / (float)h * (float)j, 1.0f};
            }
        }
    }
    image.num_rendered_frames = 1;

    return image;
}
RayImage RayTraceBVH(const RayCamera& cam, const BoundingVolumeHierarchy& bvh, const glm::mat4& inv_model_mat, uint32_t w, uint32_t h) {
    RayImage image(w, h);
    for(size_t i = 0; i < image.width * image.height; ++i) {
        image.colors[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    const float sx = 1.0f / (float)w;
    const float sy = 1.0f / (float)h;
    for(uint32_t j = 0; j < h; ++j) {
        const float py = sy * (h - 1 - j);
        for(uint32_t i = 0; i < w; ++i) {
            const float px = sx * (w - 1 - i);
            const glm::vec3 point_local = cam.bottom_left_local + glm::vec3(cam.plane_width * px, cam.plane_height * py, 0.0f);
            const glm::vec3 point = cam.pos + cam.right * point_local.x + cam.up * point_local.y + cam.forward * point_local.z;


            Ray ray = {};
            ray.dir = inv_model_mat * glm::vec4(glm::normalize(point - cam.pos), 0.0f);
            ray.origin = inv_model_mat * glm::vec4(cam.pos, 1.0f);

            RayHitResult hit_result = RayBoundingVolumeHierarchyTest(ray, bvh);
            if(hit_result.hit) {
                image.colors[j * w + i] = hit_result.col;
            }
            else {
                //image.colors[j * w + i] = {1.0f, 1.0f / (float)w * (float)i, 1.0f / (float)h * (float)j, 1.0f};
            }
        }
    }

    image.num_rendered_frames = 1;
    return image;
}
static RayHitResult RaySceneCollisionTest(const Ray& ray, const RayScene& scene) {
    RayHitResult cur_hit_result;
    cur_hit_result.hit = false;
    cur_hit_result.length = INFINITY;
    cur_hit_result.material = nullptr;
    Ray hit_ray;
    for(const RayObject& obj : scene.objects) {
        Ray cur_ray;
        cur_ray.dir = obj.inv_model_mat * glm::vec4(ray.dir, 0.0f);
        cur_ray.origin = obj.inv_model_mat * glm::vec4(ray.origin, 1.0f);
        RayHitResult hit_result = RayBoundingVolumeHierarchyTest(cur_ray, *obj.bvh);
        if(hit_result.hit && hit_result.length < cur_hit_result.length) {
            cur_hit_result = hit_result;
            cur_hit_result.material = &obj.material;
            cur_hit_result.pos = ray.origin + ray.dir * cur_hit_result.length;
            cur_hit_result.normal = glm::normalize(obj.mat * glm::vec4(hit_result.normal, 0.0f));
            hit_ray = cur_ray;
        }
    }
    return cur_hit_result;
}
static glm::vec4 GetEnvironmentLight(const Texture2D& hdr_map, const Ray& ray) {
    const glm::vec4* data = (const glm::vec4*)hdr_map.data;
    const float phi = atan2f(ray.dir.z, ray.dir.x);
    const float theta = acosf(ray.dir.y);
    const uint32_t x = (uint32_t)(((phi ) / (2.0f * M_PI)) * hdr_map.width) % hdr_map.width;
    const uint32_t y = (uint32_t)((theta / M_PI) * hdr_map.height) % hdr_map.height;
    return data[y * hdr_map.width + (hdr_map.width - 1 - x)];
}

// simple gradient
static glm::vec4 GetEnvironmentLight(const Ray& ray) {
    float sky_gradient_t = glm::pow(glm::smoothstep(0.0f, 0.4f, ray.dir.y), 0.35);
    float ground_to_sky_t = glm::smoothstep(-0.01f, 0.0f, ray.dir.y);
    glm::vec3 sky_gradient = glm::mix(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), glm::vec4(0.6f, 0.6f, 1.0f, 1.0f), sky_gradient_t);

    // Combine ground, sky, and sun
    glm::vec3 composite = glm::mix(glm::vec3(0.2f, 0.2f, 0.2f), sky_gradient, ground_to_sky_t);
    if(ray.dir.x < 0.0f) {
        return glm::vec4(composite, 1.0f);
    }
    return glm::vec4(0.0f);
}
static glm::vec4 ShootRayInScene(const Ray& ray, const RayScene& scene, const glm::vec4& start_col, const glm::vec4& start_light_col, uint32_t max_bounces) {
    glm::vec4 cur_col = start_col;
    glm::vec4 light_col = start_light_col;

    Ray main_ray = ray;
    for(uint32_t i = 0; i < max_bounces; ++i) {
        RayHitResult cur_hit_result = RaySceneCollisionTest(main_ray, scene);
        if(cur_hit_result.hit) {
            const float specular = cur_hit_result.material->specular_probability >= GetRandomFloat(0.0f, 1.0f) ? 1.0f : 0.0f;

            main_ray.origin = cur_hit_result.pos;

            //glm::vec3 diffuse_dir = glm::normalize(cur_hit_result.normal + GetRandomNormalizedVector());
            glm::vec3 diffuse_dir = GetRandomNormalizedVector();
            if(glm::dot(diffuse_dir, cur_hit_result.normal) < 0.0f) {
                diffuse_dir = -diffuse_dir;
            }
            const glm::vec3 specular_dir = glm::reflect(main_ray.dir, cur_hit_result.normal);
            main_ray.dir = glm::normalize(glm::mix(diffuse_dir, specular_dir, cur_hit_result.material->smoothness * specular));

            glm::vec3 emitted = cur_hit_result.material->emission_col * cur_hit_result.material->emission_strength;
            light_col += glm::vec4(emitted, 1.0f) * cur_col;
            cur_col *= glm::mix(cur_hit_result.col, cur_hit_result.material->specular_col, specular);

            const float p = glm::max(cur_col.r, glm::max(cur_col.g, cur_col.b));
            if(p <= 1e-10f) {
                break;
            }
            cur_col *= 1.0f / p;
        }
        else {
            if(scene.hdr_map) {
                light_col += GetEnvironmentLight(*scene.hdr_map, main_ray);
            }
            else {
                light_col += GetEnvironmentLight(main_ray) * cur_col;
            }
            break;
        }
    }
    return light_col;
}
RayImage RayTraceScene(const RayCamera& cam, const RayScene& scene, uint32_t max_bounces, uint32_t sample_count, uint32_t w, uint32_t h) {
    RayImage image(w, h);
    for(size_t i = 0; i < image.width * image.height; ++i) {
        image.colors[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    const float color_scale = 1.0f / (float)sample_count;
    const glm::vec4 start_col = {1.0f, 1.0f, 1.0f, 1.0f};
    const glm::vec4 start_light_col = {0.0f, 0.0f, 0.0f, 1.0f};

    const float sx = 1.0f / (float)w;
    const float sy = 1.0f / (float)h;
    const glm::vec2 pixel_scale = glm::vec2(sx, sy);
    for(uint32_t j = 0; j < h; ++j) {
        const float py = sy * (h - 1 - j);
        for(uint32_t i = 0; i < w; ++i) {
            const float px = sx * (w - 1 - i);
            glm::vec4 accum_col = {};
            const glm::vec3 point_local = cam.bottom_left_local + glm::vec3(cam.plane_width * px, cam.plane_height * py, 0.0f);
            const glm::vec3 point = cam.pos + cam.right * point_local.x + cam.up * point_local.y + cam.forward * point_local.z;
            for(uint32_t k = 0; k < sample_count; ++k) {
                glm::vec2 rand_vec = GetRandomPointInSquare() * pixel_scale;
                const glm::vec3 offset_point = point + cam.right * rand_vec.x + cam.up * rand_vec.y;

                Ray ray = {};
                ray.dir = glm::normalize(offset_point - cam.pos);
                ray.origin = cam.pos;
                accum_col += ShootRayInScene(ray, scene, start_col, start_light_col, max_bounces);
            }
            image.colors[j * w + i] = accum_col * color_scale;
        }
    }
    image.num_rendered_frames = sample_count;
    return image;
}
void RayTraceSceneAccumulate(RayImage& img, const RayCamera& cam, const RayScene& scene, uint32_t max_bounces, uint32_t sample_count) {
    const float color_scale = 1.0f / (float)sample_count;
    const uint32_t w = img.width;
    const uint32_t h = img.height;

    const float sx = 1.0f / (float)w;
    const float sy = 1.0f / (float)h;
    const glm::vec2 pixel_scale = glm::vec2(sx, sy);
    img.frame_scale = 1.0f / (img.num_rendered_frames + 1);
    const glm::vec4 start_col = {1.0f, 1.0f, 1.0f, 1.0f};
    const glm::vec4 start_light_col = {0.0f, 0.0f, 0.0f, 1.0f};
    

    for(uint32_t j = 0; j < h; ++j) {
        const float py = sy * (h - 1 - j);
        for(uint32_t i = 0; i < w; ++i) {
            const float px = sx * (w - 1 - i);
            glm::vec4 accum_col = {};
            const glm::vec3 point_local = cam.bottom_left_local + glm::vec3(cam.plane_width * px, cam.plane_height * py, 0.0f);
            const glm::vec3 point = cam.pos + cam.right * point_local.x + cam.up * point_local.y + cam.forward * point_local.z;
            for(uint32_t k = 0; k < sample_count; ++k) {
                glm::vec2 rand_vec = GetRandomPointInCircle() * pixel_scale;
                const glm::vec3 offset_point = point + cam.right * rand_vec.x + cam.up * rand_vec.y;

                Ray ray = {};
                ray.dir = glm::normalize(offset_point - cam.pos);
                ray.origin = cam.pos;
                accum_col += ShootRayInScene(ray, scene, start_col, start_light_col, max_bounces);
            }
            accum_col *= color_scale;

            img.colors[j * w + i] = (img.colors[j * w + i] * (1.0f - img.frame_scale) + accum_col * img.frame_scale);
        }
    }
    img.num_rendered_frames += 1;
}

void RayTraceMapper(LitObject& lit, const RayScene& scene, uint32_t max_bounces, uint32_t sample_count) {
    if(lit.scene_idx >= scene.objects.size()) {
        return;
    }
    const float color_scale = 1.0f / (float)sample_count;
    const uint32_t w = lit.lightmap.width;
    const uint32_t h = lit.lightmap.height;

    const float inv_size_x = 1.0f / (float)w;
    const float inv_size_y = 1.0f / (float)h;
    const glm::vec2 pixel_scale = glm::vec2(inv_size_x, inv_size_y);
    lit.lightmap.frame_scale = 1.0f / (lit.lightmap.num_rendered_frames + 1);

    const RayObject& obj = scene.objects.at(lit.scene_idx);
    const glm::vec4 start_col = {1.0f, 1.0f, 1.0f, 1.0f};
    const glm::vec4 start_light_col = glm::vec4(glm::vec3(obj.material.emission_col * obj.material.emission_strength), 1.0f);

    for(const auto& trig : obj.bvh->triangle_data) {
        const glm::vec2 v21_uv = trig.v2.uv - trig.v1.uv;
        const glm::vec2 v31_uv = trig.v3.uv - trig.v1.uv;
        const glm::vec3 v21_pos = trig.v2.pos - trig.v1.pos;
        const glm::vec3 v31_pos = trig.v3.pos - trig.v1.pos;
        const glm::vec3 v21_nor = trig.v2.nor - trig.v1.nor;
        const glm::vec3 v31_nor = trig.v3.nor - trig.v1.nor;
        const float v21_len = glm::length(v21_uv);
        const float v31_len = glm::length(v31_uv);
        const glm::ivec2 start_px = trig.v1.uv * glm::vec2((float)lit.lightmap.width, (float)lit.lightmap.height);
        const glm::ivec2 num_steps = {static_cast<int>(glm::max(v21_len / inv_size_x, v21_len / inv_size_y)), static_cast<int>(glm::max(v31_len / inv_size_x, v31_len / inv_size_y))};

        const float step_x = 1.0f / (float)(num_steps.x);
        const float step_y = 1.0f / (float)(num_steps.y);


        glm::vec2 cur_uv = trig.v1.uv;
        for(int y = 0; y < num_steps.y; ++y) {
            for(int x = 0; x < num_steps.x; ++x) {
                const glm::vec2 cur_uv = trig.v1.uv + v31_uv * (step_y * y) + v21_uv * (step_x * x);
                const glm::ivec2 cur_px = cur_uv * glm::vec2((float)lit.lightmap.width, (float)lit.lightmap.height);

                if(cur_px.x >= 0 && cur_px.x < w && cur_px.y >= 0 && cur_px.y < h && IsPointInTriangle(cur_uv, trig.v1.uv, trig.v2.uv, trig.v3.uv)) {
                    glm::vec4 accum_col = {};
                    for(uint32_t k = 0; k < sample_count; ++k) {
                        Ray ray;
                        float sx = step_x * (x + GetRandomFloat(0.0f, 1.0f));
                        float sy = step_y * (y + GetRandomFloat(0.0f, 1.0f));

                        ray.origin = obj.mat * glm::vec4((trig.v1.pos + v31_pos * sy + v21_pos * sx), 1.0f);
                        const glm::vec3 cur_nor = obj.mat * glm::vec4((trig.v1.nor + v31_nor * sy + v21_nor * sx), 0.0f);
                        glm::vec3 normal = GetRandomNormalizedVector();
                        if(glm::dot(cur_nor, normal) < 0.0f) {
                            normal = -normal;
                        }

                        ray.dir = normal;
                        accum_col += ShootRayInScene(ray, scene, start_col, start_light_col, max_bounces);
                    }
                    accum_col *= color_scale;
                    //lit.lightmap.colors[cur_px.y * w + cur_px.x] = glm::vec4(glm::abs(trig.v1.nor), 1.0f);
                    //lit.lightmap.colors[cur_px.y * w + cur_px.x].a = 1.0f;
                    lit.lightmap.colors[cur_px.y * w + cur_px.x] = (lit.lightmap.colors[cur_px.y * w + cur_px.x] * (1.0f - lit.lightmap.frame_scale) + accum_col * lit.lightmap.frame_scale);
                }
            }
        }
    }

    
    lit.lightmap.num_rendered_frames += 1;
}


