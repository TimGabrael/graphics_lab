#include "mesh_decimation.h"
#include "helper_math.h"
#include "util.h"
#include <iostream>
#include <algorithm>
#include <thread>

void FastDynamicMesh::Initialize(Mesh* underlying_mesh) {
    this->mesh = underlying_mesh;
    this->triangles.resize(this->mesh->triangle_count);
    for(uint32_t i = 0; i < this->mesh->triangle_count; ++i) {
        const uint32_t idx1 = this->mesh->inds[3 * i + 0];
        const uint32_t idx2 = this->mesh->inds[3 * i + 1];
        const uint32_t idx3 = this->mesh->inds[3 * i + 2];
        bool edge12_exists = false;
        bool edge13_exists = false;
        bool edge23_exists = false;
        for(size_t j = 0; j < this->edges.size(); ++j) {
            if((idx1 == this->edges.at(j).vtx_1 || idx1 == this->edges.at(j).vtx_2) && (idx2 == this->edges.at(j).vtx_1 || idx2 == this->edges.at(j).vtx_2)) {
                edge12_exists = true;
            }
            if((idx1 == this->edges.at(j).vtx_1 || idx1 == this->edges.at(j).vtx_2) && (idx3 == this->edges.at(j).vtx_1 || idx3 == this->edges.at(j).vtx_2)) {
                edge13_exists = true;   
            }
            if((idx2 == this->edges.at(j).vtx_1 || idx2 == this->edges.at(j).vtx_2) && (idx3 == this->edges.at(j).vtx_1 || idx3 == this->edges.at(j).vtx_2)) {
                edge23_exists = true;   
            }
            if(edge12_exists && edge13_exists && edge23_exists) {
                break;
            }
        }
        const glm::vec3 p1 = this->mesh->verts[idx1].pos;
        const glm::vec3 p2 = this->mesh->verts[idx2].pos;
        const glm::vec3 p3 = this->mesh->verts[idx3].pos;
        if(!edge12_exists) {
            EdgeData edge12 = {};
            edge12.vtx_1 = idx1;
            edge12.vtx_2 = idx2;
            edge12.len = glm::length(p2 - p1);
            this->edges.push_back(edge12);
        }
        if(!edge13_exists) {
            EdgeData edge13 = {};
            edge13.vtx_1 = idx1;
            edge13.vtx_2 = idx3;
            edge13.len = glm::length(p3 - p1);
            this->edges.push_back(edge13);
        }
        if(!edge23_exists) {
            EdgeData edge23 = {};
            edge23.vtx_1 = idx2;
            edge23.vtx_2 = idx3;
            edge23.len = glm::length(p3 - p2);
            this->edges.push_back(edge23);
        }
        for(uint32_t j = 0; j < this->mesh->triangle_count; ++j) {
            if(j == i) {
                continue;
            }
            const uint32_t nidx1 = this->mesh->inds[3 * j + 0];
            const uint32_t nidx2 = this->mesh->inds[3 * j + 1];
            const uint32_t nidx3 = this->mesh->inds[3 * j + 2];

            if(nidx1 == idx1 || nidx2 == idx1 || nidx3 == idx1) {
                if(nidx1 == idx2 || nidx2 == idx2 || nidx3 == idx2) {
                    this->triangles[i].neighbour_12 = j;
                }
                else if(nidx1 == idx3 || nidx2 == idx3 || nidx3 == idx3) {
                    this->triangles[i].neighbour_13 = j;
                }
            }
            else if(nidx1 == idx2 || nidx2 == idx2 || nidx3 == idx2) {
                if(nidx1 == idx3 || nidx2 == idx3 || nidx3 == idx3) {
                    this->triangles[i].neighbour_23 = j;
                }
            }
        }
        this->triangles[i].vtx_1 = idx1;
        this->triangles[i].vtx_2 = idx2;
        this->triangles[i].vtx_3 = idx3;
        this->triangles[i].area = CalcTriangleArea(p1, p2, p3);
    }
    this->vertex_data.resize(this->mesh->vertex_count);
    for(uint32_t i = 0; i < this->mesh->vertex_count; ++i) {
        for(size_t j = 0; j < this->triangles.size(); ++j) {
            const TriangleData& trig = this->triangles.at(j);
            if(trig.vtx_1 == i || trig.vtx_2 == i || trig.vtx_3 == i) {
                this->vertex_data[i].triangles.push_back(j);
            }
        }
        for(size_t j = 0; j < this->edges.size(); ++j) {
            const EdgeData& edge = this->edges.at(j);
            if(edge.vtx_1 == i || edge.vtx_2 == i) {
                this->vertex_data[i].edges.push_back(j);
            }
        }
    }
}
Mesh FastDynamicMesh::Decimate(float min_size) {
    std::vector<EdgeData> dyn_edges = this->edges;
    std::vector<Vertex> temp_vertices(this->mesh->verts, this->mesh->verts + this->mesh->vertex_count);

    std::sort(dyn_edges.begin(), dyn_edges.end(), [](const EdgeData& e1, const EdgeData& e2) {
        return e1.len < e2.len;
    });
    for(size_t i = 0; i < dyn_edges.size(); ++i) {
        // need at least 3 edges to form some kind of mesh
        if(dyn_edges.size() == 3) {
            break;
        }
        const EdgeData edge = dyn_edges.at(i);
        if(edge.len < min_size) {
            const Vertex v1 = temp_vertices.at(edge.vtx_1);
            const Vertex v2 = temp_vertices.at(edge.vtx_2);

            {
                std::vector<uint32_t> v1_connections;
                std::vector<uint32_t> v2_connections;
                std::vector<uint32_t> shared_connections;
                for(size_t j = 0; j < dyn_edges.size(); ++j) {
                    if(i == j) {
                        continue;
                    }
                    if(edge.vtx_1 == dyn_edges.at(j).vtx_1 || edge.vtx_1 == dyn_edges.at(j).vtx_2) {
                        if(edge.vtx_1 == dyn_edges.at(j).vtx_1) {
                            v1_connections.push_back(dyn_edges.at(j).vtx_2);
                        }
                        else {
                            v1_connections.push_back(dyn_edges.at(j).vtx_1);
                        }
                    }
                    if(edge.vtx_2 == dyn_edges.at(j).vtx_1 || edge.vtx_2 == dyn_edges.at(j).vtx_2) {
                        if(edge.vtx_2 == dyn_edges.at(j).vtx_1) {
                            v2_connections.push_back(dyn_edges.at(j).vtx_2);
                        }
                        else {
                            v2_connections.push_back(dyn_edges.at(j).vtx_1);
                        }
                    }
                }
                for(uint32_t v1 : v1_connections) {
                    for(uint32_t v2 : v2_connections) {
                        if(v1 == v2) {
                            shared_connections.push_back(v1);
                            break;
                        }
                    }
                }
                for(uint32_t shared : shared_connections) {
                    uint32_t min_edge_1 = UINT32_MAX;
                    float min_dist_e1 = FLT_MAX;
                    uint32_t min_edge_2 = UINT32_MAX;
                    float min_dist_e2 = FLT_MAX;
                    const glm::vec3& shared_pos = temp_vertices.at(shared).pos;
                    for(uint32_t v1_conn : v1_connections) {
                        if(v1_conn == shared) {
                            continue;
                        }
                        bool already_connected = false;
                        for(size_t j = 0; j < dyn_edges.size(); ++j) {
                            const EdgeData& ej = dyn_edges.at(j);
                            if((ej.vtx_1 == v1_conn || ej.vtx_2 == v1_conn) && (ej.vtx_1 == shared || ej.vtx_2 == shared)) {
                                already_connected = true;
                                break;
                            }
                        }
                        if(already_connected) {
                            continue;
                        }
                        const glm::vec3& v1_pos = temp_vertices.at(v1_conn).pos;
                        const float dist = glm::distance(shared_pos, v1_pos);
                        if(dist < min_dist_e1) {
                            min_dist_e1 = dist;
                            min_edge_1 = v1_conn;
                        }

                    }
                    for(uint32_t v2_conn : v2_connections) {
                        if(v2_conn == shared || v2_conn == min_edge_1) {
                            continue;
                        }
                        bool already_connected = false;
                        for(size_t j = 0; j < dyn_edges.size(); ++j) {
                            const EdgeData& ej = dyn_edges.at(j);
                            if((ej.vtx_1 == v2_conn || ej.vtx_2 == v2_conn) && (ej.vtx_1 == shared || ej.vtx_2 == shared)) {
                                already_connected = true;
                                break;
                            }
                        }
                        if(already_connected) {
                            continue;
                        }
                        const glm::vec3& v2_pos = temp_vertices.at(v2_conn).pos;
                        const float dist = glm::distance(shared_pos, v2_pos);
                        if(dist < min_dist_e2) {
                            min_dist_e2 = dist;
                            min_edge_2 = v2_conn;
                        }
                    }
                    if(min_edge_1 != UINT32_MAX) {
                        EdgeData smallest_conn_1 = {};
                        smallest_conn_1.vtx_1 = shared;
                        smallest_conn_1.vtx_2 = min_edge_1;
                        smallest_conn_1.len = min_dist_e1;
                        dyn_edges.push_back(smallest_conn_1);
                    }
                    if(min_edge_2 != UINT32_MAX) {
                        EdgeData smallest_conn_2 = {};

                        smallest_conn_2.vtx_1 = shared;
                        smallest_conn_2.vtx_2 = min_edge_2;
                        smallest_conn_2.len = min_dist_e2;

                        dyn_edges.push_back(smallest_conn_2);
                    }
                }
            }
            std::cout << "dyn_edges.size(): " << dyn_edges.size() << std::endl;

            // destroy the edge
            Vertex mid_vert = {};
            mid_vert.pos = glm::mix(v1.pos, v2.pos, 0.5f);
            mid_vert.uv = glm::mix(v1.uv, v2.uv, 0.5f);
            mid_vert.col = glm::mix(v1.col, v2.col, 0.5f);
            mid_vert.nor = glm::mix(v1.nor, v2.nor, 0.5f);

            const uint32_t new_vert_idx = temp_vertices.size();
            temp_vertices.push_back(mid_vert);




            for(size_t j = 0; j < dyn_edges.size(); ++j) {
                if(j == i) {
                    continue;
                }
                EdgeData& e1 = dyn_edges.at(j);
                if(e1.vtx_1 == edge.vtx_1 || e1.vtx_1 == edge.vtx_2) {
                    const Vertex& ov = temp_vertices.at(e1.vtx_2);
                    e1.vtx_1 = new_vert_idx;
                    e1.len = glm::distance(ov.pos, mid_vert.pos);
                }
                if(e1.vtx_2 == edge.vtx_1 || e1.vtx_2 == edge.vtx_2) {
                    const Vertex& ov = temp_vertices.at(e1.vtx_1);
                    e1.vtx_2 = new_vert_idx;
                    e1.len = glm::distance(ov.pos, mid_vert.pos);
                }
            }



            dyn_edges.erase(dyn_edges.begin() + i);

            // remove duplicate edges
            for(size_t j = 0; j < dyn_edges.size(); ++j) {
                const EdgeData& ej = dyn_edges.at(j);
                for(size_t k = 0; k < dyn_edges.size(); ++k) {
                    if(k == j) {
                        continue;
                    }
                    const EdgeData& ek = dyn_edges.at(k);

                    if((ej.vtx_1 == ek.vtx_1 || ej.vtx_1 == ek.vtx_2) && (ej.vtx_2 == ek.vtx_1 || ej.vtx_2 == ek.vtx_2)) {
                        // remove same edge
                        dyn_edges.erase(dyn_edges.begin() + k);
                        //k -= 1;
                        k = SIZE_MAX;
                    }
                }
            }

            // repeat all the calculations,
            // as a new connection might be smaller than the max_size
            i = SIZE_MAX;
            std::sort(dyn_edges.begin(), dyn_edges.end(), [](const EdgeData& e1, const EdgeData& e2) {
                return e1.len < e2.len;
            });
        }
    }
    // vertex look up
    std::unordered_map<uint32_t, uint32_t> translation_map;
    std::vector<Vertex> compacted_vertices;
    for(const EdgeData& e : dyn_edges) {
        if(translation_map.find(e.vtx_1) == translation_map.end()) {
            translation_map[e.vtx_1] = compacted_vertices.size();
            compacted_vertices.push_back(temp_vertices.at(e.vtx_1));
        }
        if(translation_map.find(e.vtx_2) == translation_map.end()) {
            translation_map[e.vtx_2] = compacted_vertices.size();
            compacted_vertices.push_back(temp_vertices.at(e.vtx_2));
        }
    }

    std::vector<uint32_t> inds;
    // rebuild a mesh from the edges
    for(size_t i = 0; i < dyn_edges.size(); ++i) {
        const EdgeData& e1 = dyn_edges.at(i);
        for(size_t j = i + 1; j < dyn_edges.size(); ++j) {
            const EdgeData& e2 = dyn_edges.at(j);
            uint32_t unrelated_v1 = UINT32_MAX;
            uint32_t unrelated_v2 = UINT32_MAX;
            uint32_t shared_v = UINT32_MAX;
            bool e1v1_related = e1.vtx_1 == e2.vtx_1 || e1.vtx_1 == e2.vtx_2;
            bool e1v2_related = e1.vtx_2 == e2.vtx_1 || e1.vtx_2 == e2.vtx_2;
            if(e1v1_related) {
                shared_v = e1.vtx_1;
                unrelated_v1 = e1.vtx_2;
                if(e1.vtx_1 == e2.vtx_1) {
                    unrelated_v2 = e2.vtx_2;
                }
                else {
                    unrelated_v2 = e2.vtx_1;
                }
            }
            else if(e1v2_related) {
                shared_v = e1.vtx_2;
                unrelated_v1 = e1.vtx_1;
                if(e1.vtx_2 == e2.vtx_1) {
                    unrelated_v2 = e2.vtx_2;
                }
                else {
                    unrelated_v2 = e2.vtx_1;
                }
            }
            if(unrelated_v1 == unrelated_v2 || unrelated_v1 == UINT32_MAX || unrelated_v2 == UINT32_MAX) {
                continue;
            }
            // check that the 2 unrelated vertices share an edge aswell
            bool unrelated_connected = false;
            for(size_t k = j + 1; k < dyn_edges.size(); ++k) {
                const EdgeData& e3 = dyn_edges.at(k);
                if((e3.vtx_1 == unrelated_v1 || e3.vtx_1 == unrelated_v2) && (e3.vtx_2 == unrelated_v1 || e3.vtx_2 == unrelated_v2)) {
                    unrelated_connected = true;
                    break;
                }
            }
            if(unrelated_connected) {
                // yippi a triangle
                uint32_t t1 = translation_map[unrelated_v1];
                uint32_t t2 = translation_map[unrelated_v2];
                uint32_t t3 = translation_map[shared_v];
                const Vertex& v1 = compacted_vertices.at(t1);
                const Vertex& v2 = compacted_vertices.at(t2);
                const Vertex& v3 = compacted_vertices.at(t3);
                const glm::vec3 d12 = v2.pos - v1.pos;
                const glm::vec3 d13 = v3.pos - v1.pos;
                const glm::vec3 normal = glm::normalize(glm::cross(d13, d12));
                if(glm::dot(normal, v1.nor) > 0.0f) {
                    std::swap(t2, t3);
                }


                inds.push_back(t1);
                inds.push_back(t2);
                inds.push_back(t3);
            }
        }
    }
    if(inds.size() == 0) {
        std::cout << "NO INDICES WERE GENERATRED" << std::endl;
        inds.push_back(0);
        inds.push_back(0);
        inds.push_back(0);
    }
    return CreateMesh(compacted_vertices.data(), inds.data(), compacted_vertices.size(), inds.size());
}
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

