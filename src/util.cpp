#include "util.h"
#include <fstream>
#include <iostream>
#include <sstream>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#include "tiny_gltf.h"
#undef min
#undef max

Mesh::~Mesh() {
    DestroyMesh(this);
}
Mesh::Mesh(Mesh&& o) {
    this->inds = o.inds;
    this->verts = o.verts;
    this->vao = o.vao;
    this->vbo = o.vbo;
    this->ibo = o.ibo;
    this->vertex_count = o.vertex_count;
    this->material_idx = o.material_idx;
    this->triangle_count = o.triangle_count;
    this->bb = o.bb;
    o.vao = INVALID_GL_HANDLE;
    o.vbo = INVALID_GL_HANDLE;
    o.ibo = INVALID_GL_HANDLE;
    o.verts = nullptr;
    o.inds = nullptr;
    o.triangle_count = 0;
    o.vertex_count = 0;
    o.material_idx = 0;
}
Mesh& Mesh::operator=(Mesh&& o) {
    DestroyMesh(this);
    this->inds = o.inds;
    this->verts = o.verts;
    this->vao = o.vao;
    this->vbo = o.vbo;
    this->ibo = o.ibo;
    this->vertex_count = o.vertex_count;
    this->material_idx = o.material_idx;
    this->triangle_count = o.triangle_count;
    this->bb = o.bb;
    o.vao = INVALID_GL_HANDLE;
    o.vbo = INVALID_GL_HANDLE;
    o.ibo = INVALID_GL_HANDLE;
    o.verts = nullptr;
    o.inds = nullptr;
    o.triangle_count = 0;
    o.vertex_count = 0;
    o.material_idx = 0;
    return *this;
}
Texture2D::Texture2D(Texture2D&& o) {
    this->id = o.id;
    this->data = o.data;
    this->type = o.type;
    this->width = o.width;
    this->height = o.height;
    this->num_channels = o.num_channels;
    o.id = INVALID_GL_HANDLE;
    o.data = nullptr;
    o.width = 0;
    o.height = 0;
    o.num_channels = 0;
}
Texture2D::~Texture2D() {
    DestroyTexture2D(this);
}
Texture2D& Texture2D::operator=(Texture2D&& o) {
    DestroyTexture2D(this);
    this->id = o.id;
    this->data = o.data;
    this->type = o.type;
    this->width = o.width;
    this->height = o.height;
    this->num_channels = o.num_channels;
    o.id = INVALID_GL_HANDLE;
    o.data = nullptr;
    o.width = 0;
    o.height = 0;
    o.num_channels = 0;
    return *this;
}

Mesh CreateMesh(const Vertex* verts, const uint32_t* inds, uint32_t vert_count, uint32_t ind_count) {
    Mesh mesh = {};
    mesh.ibo = INVALID_GL_HANDLE;
    mesh.bb.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    mesh.bb.min = {FLT_MAX, FLT_MAX, FLT_MAX};

    glCreateVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vert_count, verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nor));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));

    for(uint32_t i = 0; i < vert_count; ++i) {
        mesh.bb.min = glm::min(mesh.bb.min, verts[i].pos);
        mesh.bb.max = glm::max(mesh.bb.max, verts[i].pos);
    }
    mesh.verts = new Vertex[vert_count];
    memcpy(mesh.verts, verts, sizeof(Vertex) * vert_count);
    mesh.vertex_count = vert_count;

    if(inds) {
        glGenBuffers(1, &mesh.ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * ind_count, inds, GL_STATIC_DRAW);
        mesh.inds = new uint32_t[ind_count];
        memcpy(mesh.inds, inds, sizeof(uint32_t) * ind_count);
        mesh.triangle_count = ind_count / 3;
    }
    else {
        mesh.triangle_count = mesh.vertex_count / 3;
    }
    glBindVertexArray(0);
    return mesh;
}
GLTFLoadData LoadGLTFLoadData(const char* filename) {
    GLTFLoadData loaded = {};

    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;
    if(!loader.LoadBinaryFromFile(&model, &err, &warn, filename)) {
        std::cout << "TinyGLTF loader failed to load binary from file: " << filename << std::endl;
        std::cout << "Err: " << err << std::endl;
        return loaded;
    }
    if(!warn.empty()) {
        std::cout << "LoadGLTFMesh Warning: " << warn << std::endl;
    }

    for(const auto& material : model.materials) {
        GLTFLoadData::Material mat = {};
        mat.base_color.idx = material.pbrMetallicRoughness.baseColorTexture.index;
        mat.base_color.tex_coord_idx = material.pbrMetallicRoughness.baseColorTexture.texCoord;
        for(auto& ex : material.pbrMetallicRoughness.baseColorTexture.extensions) {
            if(ex.first == "KHR_texture_transform") {
                if(ex.second.IsObject()) {
                    for(auto& key : ex.second.Keys()) {
                        if(key == "offset") {
                            auto& offset_arr = ex.second.Get(key);
                            auto& o1 = offset_arr.Get(0);
                            auto& o2 = offset_arr.Get(1);
                            mat.base_color.transform.offset = {o1.GetNumberAsDouble(), o2.GetNumberAsDouble()};
                        }
                        else if(key == "scale") {
                            auto& scale_arr = ex.second.Get(key);
                            auto& s1 = scale_arr.Get(0);
                            auto& s2 = scale_arr.Get(1);
                            mat.base_color.transform.scale = {s1.GetNumberAsDouble(), s2.GetNumberAsDouble()};
                        }
                    }
                }
            }
        }
        loaded.materials.push_back(mat);
    }
    for(const auto& tex : model.images) {
        if(tex.bits == 8 && tex.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            loaded.textures.push_back(CreateTexture2D((const uint32_t*)tex.image.data(), tex.width, tex.height));
        }
    }

    for(const auto& m : model.meshes) {
        for(const auto& prim : m.primitives) {
            const auto& index_accessor = model.accessors[prim.indices];
            const auto& index_buffer_view = model.bufferViews[index_accessor.bufferView];
            const auto& index_buffer = model.buffers[index_buffer_view.buffer];

            uint32_t* inds = new uint32_t[index_accessor.count];
            uint32_t vtx_count = 0;
            uint32_t idx_count = index_accessor.count;
            int index_buffer_stride = index_accessor.ByteStride(index_buffer_view);
            for(uint32_t i = 0; i < idx_count; ++i) {
                const uint8_t* data = reinterpret_cast<const uint8_t*>(index_buffer.data.data() + index_accessor.byteOffset + index_buffer_view.byteOffset);
                if(index_buffer_stride == 1) {
                    inds[i] = (uint32_t)*(const uint8_t*)(data + index_buffer_stride * i);
                }
                else if(index_buffer_stride == 2) {
                    inds[i] = (uint32_t)*(const uint16_t*)(data + index_buffer_stride * i);
                }
                else if(index_buffer_stride == 4) {
                    inds[i] = (uint32_t)*(const uint32_t*)(data + index_buffer_stride * i);
                }
            }
            Vertex* verts = nullptr;
            for(const auto& attrib : prim.attributes) {
                const auto& accessor = model.accessors[attrib.second];
                if(vtx_count == 0 && accessor.count > 0) {
                    vtx_count = accessor.count;
                    verts = new Vertex[vtx_count];
                    memset(verts, 0, sizeof(Vertex) * vtx_count);
                    vtx_count = accessor.count;
                    for(uint32_t i = 0; i < vtx_count; ++i) {
                        verts[i].col = glm::vec4(1.0f);
                    }
                }
                const auto& buffer_view = model.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
                const int buffer_stride = accessor.ByteStride(buffer_view);
                const uint8_t* buf = reinterpret_cast<const uint8_t*>(buffer.data.data() + accessor.byteOffset + buffer_view.byteOffset);
                if(attrib.first == "POSITION") {
                    for(uint32_t i = 0; i < vtx_count; ++i) {
                        verts[i].pos = *(const glm::vec3*)(buf + buffer_stride * i);
                    }
                }
                else if(attrib.first == "NORMAL") {
                    for(uint32_t i = 0; i < vtx_count; ++i) {
                        verts[i].nor = *(const glm::vec3*)(buf + buffer_stride * i);
                    }
                }
                else if(attrib.first == "TEXCOORD_0") {
                    for(uint32_t i = 0; i < vtx_count; ++i) {
                        verts[i].uv = *(const glm::vec2*)(buf + buffer_stride * i);
                    }
                    if(prim.material < loaded.materials.size() && loaded.materials.at(prim.material).base_color.tex_coord_idx == 0) {
                        const auto& transform = loaded.materials.at(prim.material).base_color.transform;
                        for(uint32_t i = 0; i < vtx_count; ++i) {
                            verts[i].uv = verts[i].uv * transform.scale + transform.offset;
                        }
                    }
                }
                else {
                    std::cout << "[warning] vertex attribute is not being parsed: " << attrib.first << std::endl;
                }
            }

            loaded.meshes.push_back({CreateMesh(verts, inds, vtx_count, idx_count), static_cast<uint32_t>(prim.material)});
            delete[] verts;
            delete[] inds;
        }
    }

    return loaded;
}
void DestroyGLTFLoadData(GLTFLoadData* load) {
    load->textures.clear();
    load->meshes.clear();
    load->materials.clear();
}
void DestroyMesh(Mesh* mesh) {
    if(mesh->vao != INVALID_GL_HANDLE) {
        glDeleteVertexArrays(1, &mesh->vao);
    }
    if(mesh->vbo != INVALID_GL_HANDLE) {
        glDeleteBuffers(1, &mesh->vbo);
    }
    if(mesh->ibo != INVALID_GL_HANDLE) {
        glDeleteBuffers(1, &mesh->ibo);
    }
    mesh->vao = INVALID_GL_HANDLE;
    mesh->vbo = INVALID_GL_HANDLE;
    mesh->ibo = INVALID_GL_HANDLE;
    if(mesh->verts) {
        delete[] mesh->verts;
        mesh->verts = nullptr;
    }
    mesh->vertex_count = 0;
    if(mesh->inds) {
        delete[] mesh->inds;
        mesh->inds = nullptr;
    }
    mesh->triangle_count = 0;
    mesh->bb = {};
}
Texture2D CreateTexture2D(const uint32_t* data, uint32_t width, uint32_t height) {
    Texture2D tex = {};
    tex.data = malloc(sizeof(uint32_t) * width * height);
    tex.type = Texture2D::DataType::Uint;
    tex.width = width;
    tex.height = height;
    tex.num_channels = 4;
    memcpy(tex.data, data, width * height * sizeof(uint32_t));

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return tex;
}
Texture2D CreateTexture2D(const glm::vec4* data, uint32_t width, uint32_t height) {
    Texture2D tex = {};
    tex.data = malloc(sizeof(*data) * width * height);
    tex.type = Texture2D::DataType::Float;
    tex.width = width;
    tex.height = height;
    tex.num_channels = 4;
    memcpy(tex.data, data, width * height * sizeof(uint32_t));

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, data);
    return tex;
}
void DestroyTexture2D(Texture2D* tex) {
    if(tex->data) {
        free(tex->data);
        tex->data = nullptr;
    }
    if(tex->id != INVALID_GL_HANDLE) {
        glDeleteTextures(1, &tex->id);
        tex->id = INVALID_GL_HANDLE;
    }
    tex->width = 0;
    tex->height = 0;
}


std::string ReadShaderSource(const std::string& file_path) {
    std::ifstream shaderFile(file_path);
    if (!shaderFile.is_open()) {
        std::cout << "Failed to open shader file: " << file_path << std::endl;
        return "";
    }
    std::stringstream shader_stream;
    shader_stream << shaderFile.rdbuf();
    return shader_stream.str();
}
GLuint CompileShader(GLenum shaderType, const char* source) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint info_log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_len);
        std::string infoLog(info_log_len, ' ');
        glGetShaderInfoLog(shader, info_log_len, &info_log_len, &infoLog[0]);
        std::cout << "Error compiling shader: " << infoLog << std::endl;
        glDeleteShader(shader);
        return INVALID_GL_HANDLE;
    }

    return shader;
}

GLuint LoadProgram(const char* vs, const char* fs) {
    GLuint vs_handle = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint fs_handle = CompileShader(GL_FRAGMENT_SHADER, fs);
    if(vs_handle == INVALID_GL_HANDLE || fs_handle == INVALID_GL_HANDLE) {
        glDeleteShader(vs_handle);
        glDeleteShader(fs_handle);
        return INVALID_GL_HANDLE;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs_handle);
    glAttachShader(program, fs_handle);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glDeleteShader(vs_handle);
        glDeleteShader(fs_handle);
        GLint info_log_len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_len);
        std::string infoLog(info_log_len, ' ');
        glGetProgramInfoLog(program, info_log_len, &info_log_len, &infoLog[0]);
        std::cerr << "Error linking shader program: " << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    glDeleteShader(vs_handle);
    glDeleteShader(fs_handle);

    return program;
}
GLuint LoadProgramFromFile(const char* vs, const char* fs) {
    std::string vs_source = ReadShaderSource(vs);
    std::string fs_source = ReadShaderSource(fs);
    if(vs_source == "" || fs_source == "") {
        return INVALID_GL_HANDLE;
    }
    return LoadProgram(vs_source.c_str(), fs_source.c_str());
}

void GenerateCube(std::vector<Vertex>& verts, std::vector<uint32_t>& inds) {
    const std::vector<Vertex> vertices = {
        {glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)},
        {glm::vec3( 0.5f, -0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)},
        {glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)},
        {glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)},

        {glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)},
        {glm::vec3( 0.5f, -0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 1.0f, 1.0f)},
        {glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)},
        {glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)}
    };
    const std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,
        1, 5, 6, 6, 2, 1,
        7, 6, 5, 5, 4, 7,
        4, 0, 3, 3, 7, 4,
        3, 2, 6, 6, 7, 3,
        4, 5, 1, 1, 0, 4
    };
    verts.reserve(verts.size() + vertices.size());
    inds.reserve(inds.size() + indices.size());
    for(const auto& v : vertices) {
        verts.push_back(v);
    }
    for(uint32_t i : indices) {
        inds.push_back(i);
    }
}
void GenerateCube(std::vector<Vertex>& verts, std::vector<uint32_t>& inds, const glm::vec3& size, const glm::vec4 col[6]) {
    std::vector<Vertex> vertices = {
        // 0
        {glm::vec3(-0.5f, -0.5f,  0.5f) * size, glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec2(1.0f, 0.0f), col[0]},
        {glm::vec3(-0.5f, -0.5f,  0.5f) * size, glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 0.0f), col[1]},
        {glm::vec3(-0.5f, -0.5f,  0.5f) * size, glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec2(0.0f, 0.0f), col[2]},

        // 3
        {glm::vec3( 0.5f, -0.5f,  0.5f) * size, glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec2(0.0f, 0.0f), col[0]},
        {glm::vec3( 0.5f, -0.5f,  0.5f) * size, glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 0.0f), col[3]},
        {glm::vec3( 0.5f, -0.5f,  0.5f) * size, glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec2(1.0f, 0.0f), col[2]},

        // 6
        {glm::vec3( 0.5f,  0.5f,  0.5f) * size, glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec2(1.0f, 1.0f), col[0]},
        {glm::vec3( 0.5f,  0.5f,  0.5f) * size, glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 1.0f), col[3]},
        {glm::vec3( 0.5f,  0.5f,  0.5f) * size, glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec2(1.0f, 1.0f), col[4]},

        // 9
        {glm::vec3(-0.5f,  0.5f,  0.5f) * size, glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec2(0.0f, 1.0f), col[0]},
        {glm::vec3(-0.5f,  0.5f,  0.5f) * size, glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 1.0f), col[1]},
        {glm::vec3(-0.5f,  0.5f,  0.5f) * size, glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec2(0.0f, 1.0f), col[4]},

        // 12
        {glm::vec3(-0.5f, -0.5f, -0.5f) * size, glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec2(0.0f, 0.0f), col[5]},
        {glm::vec3(-0.5f, -0.5f, -0.5f) * size, glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 0.0f), col[1]},
        {glm::vec3(-0.5f, -0.5f, -0.5f) * size, glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec2(0.0f, 1.0f), col[2]},

        // 15
        {glm::vec3( 0.5f, -0.5f, -0.5f) * size, glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec2(1.0f, 0.0f), col[5]},
        {glm::vec3( 0.5f, -0.5f, -0.5f) * size, glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 0.0f), col[3]},
        {glm::vec3( 0.5f, -0.5f, -0.5f) * size, glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec2(0.0f, 0.0f), col[2]},

        // 18
        {glm::vec3( 0.5f,  0.5f, -0.5f) * size, glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec2(1.0f, 1.0f), col[5]},
        {glm::vec3( 0.5f,  0.5f, -0.5f) * size, glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec2(0.0f, 1.0f), col[3]},
        {glm::vec3( 0.5f,  0.5f, -0.5f) * size, glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec2(1.0f, 0.0f), col[4]},

        // 21
        {glm::vec3(-0.5f,  0.5f, -0.5f) * size, glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec2(0.0f, 1.0f), col[5]},
        {glm::vec3(-0.5f,  0.5f, -0.5f) * size, glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec2(1.0f, 1.0f), col[1]},
        {glm::vec3(-0.5f,  0.5f, -0.5f) * size, glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec2(0.0f, 0.0f), col[4]},
    };
    std::vector<uint32_t> indices = {
        0, 3, 6, 6, 9, 0,
        4, 16, 19, 19, 7, 4,
        21, 18, 15, 15, 12, 21,
        13, 1, 10, 10, 22, 13,

        11, 8, 20, 20, 23, 11,
        14, 17, 5, 5, 2, 14
    };
    for(size_t i = 0; i < indices.size() / 3; ++i) {
        const uint32_t idx_1 = indices.at(3 * i + 0);
        const uint32_t idx_2 = indices.at(3 * i + 1);
        const uint32_t idx_3 = indices.at(3 * i + 2);



    }
    verts.reserve(verts.size() + vertices.size());
    inds.reserve(inds.size() + indices.size());
    for(const auto& v : vertices) {
        verts.push_back(v);
    }
    for(uint32_t i : indices) {
        inds.push_back(i);
    }
}
