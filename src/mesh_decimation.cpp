#include "mesh_decimation.h"
#include "helper_math.h"
#include "util.h"
#include <iostream>
#include <algorithm>

glm::vec2 GetMeshMinMaxTrianglesSizes(const Mesh* input) {
    float min_trig_area = FLT_MAX;
    float max_trig_area = -FLT_MAX;
    for(uint32_t i = 0; i < input->triangle_count; ++i) {
        if(input->inds) {
            const glm::vec3& p1 = input->verts[input->inds[3 * i + 0]].pos;
            const glm::vec3& p2 = input->verts[input->inds[3 * i + 1]].pos;
            const glm::vec3& p3 = input->verts[input->inds[3 * i + 2]].pos;
            const float area = CalcTriangleArea(p1, p2, p3);
            min_trig_area = std::min(min_trig_area, area);
            max_trig_area = std::max(max_trig_area, area);
        }
        else {
            const glm::vec3& p1 = input->verts[3 * i + 0].pos;
            const glm::vec3& p2 = input->verts[3 * i + 1].pos;
            const glm::vec3& p3 = input->verts[3 * i + 2].pos;
            const float area = CalcTriangleArea(p1, p2, p3);
            min_trig_area = std::min(min_trig_area, area);
            max_trig_area = std::max(max_trig_area, area);
        }
    }
    return {min_trig_area, max_trig_area};
}

Mesh DecimateMesh(const Mesh* input, float min_triangle_size) {
    Mesh output = {};
    output.vao = INVALID_GL_HANDLE;
    output.vbo = INVALID_GL_HANDLE;
    output.ibo = INVALID_GL_HANDLE;
    if(!input->inds) {
        // the whole thing i'm doing is relying on shared vertices, so... yeah that ain't working
        return output;
    }

    struct SimpleTriangle {
        uint32_t idx_1;
        uint32_t idx_2;
        uint32_t idx_3;
    };
    std::vector<SimpleTriangle> new_mesh;
    std::vector<uint32_t> removed_vertices;

    for(uint32_t i = 0; i < input->triangle_count; ++i) {
        const uint32_t idx1 = input->inds[3 * i + 0];
        const uint32_t idx2 = input->inds[3 * i + 1];
        const uint32_t idx3 = input->inds[3 * i + 2];

        const glm::vec3& p1 = input->verts[idx1].pos;
        const glm::vec3& p2 = input->verts[idx2].pos;
        const glm::vec3& p3 = input->verts[idx3].pos;
        const float area = CalcTriangleArea(p1, p2, p3);
        if(area < min_triangle_size) {
            std::vector<uint32_t> n1 = {idx2, idx3};
            std::vector<uint32_t> n2 = {idx1, idx3};
            std::vector<uint32_t> n3 = {idx1, idx2};

            std::vector<SimpleTriangle> trigs;
            auto remove_duplicate = [](const std::vector<uint32_t>& n) -> std::vector<uint32_t> {
                std::vector<uint32_t> out;
                for(auto& ne : n) {
                    bool exists = false;
                    for(auto& nei : out) {
                        if(nei == ne) {
                            exists = true;
                            break;
                        }
                    }
                    if(!exists) {
                        out.push_back(ne);
                    }
                }
                return out;
            };
            bool border_contains_already_removed = false;

            // find all neighbours of the vertices p1,p2,p3
            for(uint32_t j = 0; j < input->triangle_count; ++j) {
                const uint32_t sub_idx1 = input->inds[3 * j + 0];
                const uint32_t sub_idx2 = input->inds[3 * j + 1];
                const uint32_t sub_idx3 = input->inds[3 * j + 2];
                bool is_part_of_the_loop = false;
                //for(uint32_t v : removed_vertices) {
                //    if(sub_idx1 == v || sub_idx2 == v || sub_idx3 == v) {
                //        //border_contains_already_removed = true;
                //        break;
                //    }
                //}

                if(sub_idx1 == idx1) { n1.push_back(sub_idx2); n1.push_back(sub_idx3); is_part_of_the_loop = true; }
                if(sub_idx2 == idx1) { n1.push_back(sub_idx1); n1.push_back(sub_idx3); is_part_of_the_loop = true; }
                if(sub_idx3 == idx1) { n1.push_back(sub_idx1); n1.push_back(sub_idx2); is_part_of_the_loop = true; }

                if(sub_idx1 == idx2) { n2.push_back(sub_idx2); n2.push_back(sub_idx3); is_part_of_the_loop = true; }
                if(sub_idx2 == idx2) { n2.push_back(sub_idx1); n2.push_back(sub_idx3); is_part_of_the_loop = true; }
                if(sub_idx3 == idx2) { n2.push_back(sub_idx1); n2.push_back(sub_idx2); is_part_of_the_loop = true; }

                if(sub_idx1 == idx3) { n3.push_back(sub_idx2); n3.push_back(sub_idx3); is_part_of_the_loop = true; }
                if(sub_idx2 == idx3) { n3.push_back(sub_idx1); n3.push_back(sub_idx3); is_part_of_the_loop = true; }
                if(sub_idx3 == idx3) { n3.push_back(sub_idx1); n3.push_back(sub_idx2); is_part_of_the_loop = true; }

                if(is_part_of_the_loop) {
                    trigs.push_back({sub_idx1, sub_idx2, sub_idx3});
                }
            }
            n1 = remove_duplicate(n1);
            n2 = remove_duplicate(n2);
            n3 = remove_duplicate(n3);

            std::sort(n1.begin(), n1.end(), [p1, input](uint32_t f, uint32_t s) {
                    const glm::vec3 pf = input->verts[f].pos - p1;
                    const glm::vec3 ps = input->verts[s].pos - p1;
                    float af = AngleBetweenVectors(pf, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f});
                    float as = AngleBetweenVectors(ps, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f});
                    return af > as;
            });
            
            if(n1.size() < 3 || border_contains_already_removed) {
                //std::cout << "not enough neighbours for this vertex: " << idx1 << std::endl;
                new_mesh.push_back({idx1, idx2, idx3});
            }
            else {
                //new_mesh.push_back({idx1, idx2, idx3});
                // test this by removing the first vertex
                removed_vertices.push_back(idx1);
                size_t start_idx = 0;
                for(size_t l = 0; l < n1.size() - 2; ++l) {
                    const size_t trig_1 = (start_idx + (l + 1)) % n1.size();
                    const size_t trig_2 = (start_idx + (l + 2)) % n1.size();
                    new_mesh.push_back({n1.at(start_idx), n1.at(trig_1), n1.at(trig_2)});
                }
            }
        }
        else {
            new_mesh.push_back({idx1, idx2, idx3});
        }
    }

    uint32_t* idx_translation = new uint32_t[input->triangle_count * 3];
    memset(idx_translation, 0xFF, sizeof(uint32_t) * input->triangle_count * 3);

    Vertex* verts = new Vertex[input->vertex_count];
    memset(verts, 0, sizeof(Vertex) * input->vertex_count);
    
    std::vector<uint32_t> inds;
    inds.reserve(input->triangle_count * 3);

    uint32_t cur_vertex_idx = 0;
    for(auto& trig : new_mesh) {
        bool is_valid = true;
        for(uint32_t v : removed_vertices) {
            if(trig.idx_1 == v || trig.idx_2 == v || trig.idx_3 == v) {
                is_valid = false;
            }
        }
        if(is_valid) {
            if(idx_translation[trig.idx_1] == UINT32_MAX) {
                idx_translation[trig.idx_1] = cur_vertex_idx++;
                verts[idx_translation[trig.idx_1]] = input->verts[trig.idx_1];
            }
            if(idx_translation[trig.idx_2] == UINT32_MAX) {
                idx_translation[trig.idx_2] = cur_vertex_idx++;
                verts[idx_translation[trig.idx_2]] = input->verts[trig.idx_2];
            }
            if(idx_translation[trig.idx_3] == UINT32_MAX) {
                idx_translation[trig.idx_3] = cur_vertex_idx++;
                verts[idx_translation[trig.idx_3]] = input->verts[trig.idx_3];
            }
            inds.push_back(idx_translation[trig.idx_1]);
            inds.push_back(idx_translation[trig.idx_2]);
            inds.push_back(idx_translation[trig.idx_3]);
        }
    }

    std::cout << "removed_vertex_count: " << removed_vertices.size() << std::endl;
    std::cout << "vert_count/indx_count: " << cur_vertex_idx << ", " << inds.size() << std::endl;
    std::cout << "input vert_count/indx_count: " << input->vertex_count << ", " << input->triangle_count * 3 << std::endl;

    output = CreateMesh(verts, inds.data(), cur_vertex_idx, inds.size());
    output.material_idx = input->material_idx;

    delete[] verts;


    return output;
}

