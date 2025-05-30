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
#include "xatlas.h"
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
RenderTexture::RenderTexture(RenderTexture&& o) {
    this->framebuffer_id = o.framebuffer_id;
    for(size_t i = 0; i < ARRSIZE(this->colorbuffer_ids); ++i) {
        this->colorbuffer_ids[i] = o.colorbuffer_ids[i];
        o.colorbuffer_ids[i] = INVALID_GL_HANDLE;
    }
    this->depthbuffer_id = o.depthbuffer_id;
    this->width = o.width;
    this->height = o.height;
    o.framebuffer_id = INVALID_GL_HANDLE;
    o.depthbuffer_id = INVALID_GL_HANDLE;
    o.width = 0;
    o.height = 0;
}
RenderTexture::~RenderTexture() {
    DestroyRenderTexture(this);
}
RenderTexture& RenderTexture::operator=(RenderTexture&& o) {
    DestroyRenderTexture(this);
    this->framebuffer_id = o.framebuffer_id;
    for(size_t i = 0; i < ARRSIZE(this->colorbuffer_ids); ++i) {
        this->colorbuffer_ids[i] = o.colorbuffer_ids[i];
        o.colorbuffer_ids[i] = INVALID_GL_HANDLE;
    }
    this->depthbuffer_id = o.depthbuffer_id;
    this->width = o.width;
    this->height = o.height;
    o.framebuffer_id = INVALID_GL_HANDLE;
    o.depthbuffer_id = INVALID_GL_HANDLE;
    o.width = 0;
    o.height = 0;
    return *this;
}
void RenderTexture::Begin(const glm::vec4& clear_col, float depth_clear_val, uint32_t stencil_clear_val) const {
    glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer_id);
    glViewport(0, 0, this->width, this->height);
    glClearColor(clear_col.r, clear_col.g, clear_col.b, clear_col.a);
    glClearDepthf(depth_clear_val);
    glClearStencil(stencil_clear_val);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}
void RenderTexture::End() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    tex.data = malloc(sizeof(*data) * width * height);
    tex.type = Texture2D::DataType::Uint;
    tex.width = width;
    tex.height = height;
    tex.num_channels = 4;
    memcpy(tex.data, data, width * height * sizeof(*data));

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
    memcpy(tex.data, data, width * height * sizeof(*data));

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, data);
    return tex;
}
Texture2D CreateTexture2D(const float* data, uint32_t width, uint32_t height) {
    Texture2D tex = {};
    tex.data = malloc(sizeof(*data) * width * height);
    tex.type = Texture2D::DataType::Float;
    tex.width = width;
    tex.height = height;
    tex.num_channels = 1;
    memcpy(tex.data, data, width * height * sizeof(*data));

    glGenTextures(1, &tex.id);
    glBindTexture(GL_TEXTURE_2D, tex.id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data);
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
RenderTexture CreateDepthTexture(uint32_t width, uint32_t height) {
    RenderTexture tex;
    tex.width = width;
    tex.height = height;
    glGenFramebuffers(1, &tex.framebuffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, tex.framebuffer_id);

    glGenTextures(1, &tex.depthbuffer_id);
    glBindTexture(GL_TEXTURE_2D, tex.depthbuffer_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex.depthbuffer_id, 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "failed to create Framebuffer in: CreateDepthTexture" << std::endl;
        Sleep(10000);
        DestroyRenderTexture(&tex);
    }
    return tex;
}
RenderTexture CreateRenderTexture(uint32_t width, uint32_t height) {
    RenderTexture tex;
    tex.width = width;
    tex.height = height;
    glGenFramebuffers(1, &tex.framebuffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, tex.framebuffer_id);

    glGenTextures(1, &tex.colorbuffer_ids[0]);
    glBindTexture(GL_TEXTURE_2D, tex.colorbuffer_ids[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenTextures(1, &tex.depthbuffer_id);
    glBindTexture(GL_TEXTURE_2D, tex.depthbuffer_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex.depthbuffer_id, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.colorbuffer_ids[0], 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "failed to create Framebuffer in: CreateRenderTexture" << std::endl;
        Sleep(10000);
        DestroyRenderTexture(&tex);
    }
    return tex;
}
void DestroyRenderTexture(RenderTexture* tex) {
    if(tex->framebuffer_id != INVALID_GL_HANDLE) {
        glDeleteFramebuffers(1, &tex->framebuffer_id);
        tex->framebuffer_id = INVALID_GL_HANDLE;
    }
    uint32_t colorbuffer_count = 0;
    for(size_t i = 0; i < ARRSIZE(tex->colorbuffer_ids); ++i) {
        if(tex->colorbuffer_ids[i] != INVALID_GL_HANDLE) {
            colorbuffer_count += 1;
        }
    }
    glDeleteTextures(colorbuffer_count, tex->colorbuffer_ids);
    for(size_t i = 0; i < ARRSIZE(tex->colorbuffer_ids); ++i) {
        tex->colorbuffer_ids[i] = INVALID_GL_HANDLE;
    }
    if(tex->depthbuffer_id != INVALID_GL_HANDLE) {
        glDeleteTextures(1, &tex->depthbuffer_id);
        tex->depthbuffer_id = INVALID_GL_HANDLE;
    }
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
GLuint LoadProgram(const char* cs) {
    GLuint cs_handle = CompileShader(GL_COMPUTE_SHADER, cs);
    if(cs_handle == INVALID_GL_HANDLE) {
        glDeleteShader(cs_handle);
        return INVALID_GL_HANDLE;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, cs_handle);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glDeleteShader(cs_handle);
        GLint info_log_len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_len);
        std::string infoLog(info_log_len, ' ');
        glGetProgramInfoLog(program, info_log_len, &info_log_len, &infoLog[0]);
        std::cerr << "Error linking shader program: " << infoLog << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    glDeleteShader(cs_handle);
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
GLuint LoadComputeProgramFromFile(const char* cs) {
    const std::string cs_source = ReadShaderSource(cs);
    if(cs_source == "") {
        return INVALID_GL_HANDLE;
    }
    return LoadProgram(cs_source.c_str());
}

void DrawHDR(const Texture2D& tex, const glm::mat4& proj_matrix, const glm::mat4& view_matrix) {
    static GLuint hdr_program = INVALID_GL_HANDLE;
    static GLuint hdr_vao = INVALID_GL_HANDLE;
    static GLuint hdr_vbo = INVALID_GL_HANDLE;
    static GLuint hdr_proj_mat_loc = INVALID_GL_HANDLE;
    static GLuint hdr_view_mat_loc = INVALID_GL_HANDLE;

    if(hdr_program == INVALID_GL_HANDLE) {
        hdr_program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/environment.vs").c_str(), TranslateRelativePath("../../assets/shaders/environment.fs").c_str());

        static glm::vec2 positions[] = {
            {-1.0f,  1.0f},
            {-1.0f, -1.0f},
            { 1.0f, -1.0f},
            {-1.0f,  1.0f},
            { 1.0f, -1.0f},
            { 1.0f,  1.0f},
        };

        glCreateVertexArrays(1, &hdr_vao);
        glBindVertexArray(hdr_vao);
        glGenBuffers(1, &hdr_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, hdr_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * ARRSIZE(positions), positions, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
        hdr_proj_mat_loc = glGetUniformLocation(hdr_program, "invProjMatrix");
        hdr_view_mat_loc = glGetUniformLocation(hdr_program, "viewMatrix");
    }

    glUseProgram(hdr_program);
    glBindVertexArray(hdr_vao);
    const glm::mat4 inv_proj_mat = glm::inverse(proj_matrix);
    glUniformMatrix4fv(hdr_proj_mat_loc, 1, GL_FALSE, (const float*)&inv_proj_mat);
    glUniformMatrix4fv(hdr_view_mat_loc, 1, GL_FALSE, (const float*)&view_matrix);

    glBindTexture(GL_TEXTURE_2D, tex.id);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
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

void GenerateSphere(std::vector<Vertex>& verts, std::vector<uint32_t>& inds, const glm::vec3& pos, const glm::vec3& size, const glm::vec4& col, uint32_t segments) {
    verts.reserve(verts.size() + segments * segments);
    inds.reserve(inds.size() + 6 * segments * segments);
    const size_t vert_count = verts.size();
    const float dtheta = static_cast<float>(M_PI) / static_cast<float>(segments);
    const float dphi = dtheta * 2.0f;

    for(uint32_t i = 0; i < segments; ++i) {
        const float cur_phi = i * dphi;
        for(uint32_t j = 0; j < segments; ++j) {
            const float cur_theta = j * dtheta;

            const glm::vec3 normal = {
                sinf(cur_theta) * cosf(cur_phi),
                sinf(cur_theta) * sinf(cur_phi),
                cosf(cur_theta)
            };
            verts.push_back({ .pos = normal * size + pos, .nor = normal, .uv = {}, .col = col });

            const uint32_t nj = (j + 1) % segments;
            const uint32_t ni = (i + 1) % segments;

            if(j < segments - 1) {
                inds.push_back(i * segments + j + vert_count);
                inds.push_back(ni * segments + nj + vert_count);
                inds.push_back(ni * segments + j + vert_count);
            }
            if(j < segments - 1) {
                inds.push_back(i * segments + j + vert_count);
                inds.push_back(i * segments + nj + vert_count);
                inds.push_back(ni * segments + nj + vert_count);
            }
        }
    }
}
void GenerateIcoSphere(std::vector<Vertex>& verts, std::vector<uint32_t>& inds, const glm::vec3& pos, const glm::vec3& size, const glm::vec4& col, bool flat) {
    static constexpr glm::vec3 precalc_positions[12] = {
        glm::vec3 {  0.0000000e+00,  0.0000000e+00,  1.0000000e+00 },
        glm::vec3 {  8.9442718e-01,  0.0000000e+00,  4.4721359e-01 },
        glm::vec3 {  2.7639320e-01,  8.5065079e-01,  4.4721359e-01 },
        glm::vec3 { -7.2360682e-01,  5.2573109e-01,  4.4721359e-01 },
        glm::vec3 { -7.2360682e-01, -5.2573109e-01,  4.4721359e-01 },
        glm::vec3 {  2.7639320e-01, -8.5065079e-01,  4.4721359e-01 },
        glm::vec3 {  7.2360682e-01,  5.2573109e-01, -4.4721359e-01 },
        glm::vec3 { -2.7639320e-01,  8.5065079e-01, -4.4721359e-01 },
        glm::vec3 { -8.9442718e-01,  1.0953574e-16, -4.4721359e-01 },
        glm::vec3 { -2.7639320e-01, -8.5065079e-01, -4.4721359e-01 },
        glm::vec3 {  7.2360682e-01, -5.2573109e-01, -4.4721359e-01 },
        glm::vec3 {  0.0000000e+00,  0.0000000e+00, -1.0000000e+00 }
    };
    static constexpr uint32_t precalc_indices[20 * 3] = {
        0,  1,  2,
        0,  2,  3,
        0,  3,  4,
        0,  4,  5,
        0,  5,  1,
        1,  6,  2,
        2,  7,  3,
        3,  8,  4,
        4,  9,  5,
        5, 10,  1,
        2,  6,  7,
        3,  7,  8,
        4,  8,  9,
        5,  9, 10,
        1, 10,  6,
        6, 11,  7,
        7, 11,  8,
        8, 11,  9,
        9, 11, 10,
        10, 11,  6
    };

    const uint32_t prev_vtx_count = verts.size();
    if(flat) {
        verts.reserve(ARRSIZE(precalc_indices));
        inds.reserve(ARRSIZE(precalc_indices));

        for(size_t i = 0; i < ARRSIZE(precalc_indices) / 3; ++i) {
            const glm::vec3& p1 = precalc_positions[precalc_indices[i * 3 + 0]];
            const glm::vec3& p2 = precalc_positions[precalc_indices[i * 3 + 1]];
            const glm::vec3& p3 = precalc_positions[precalc_indices[i * 3 + 2]];
            const glm::vec3 nor = glm::normalize(glm::cross(p2 - p1, p3 - p1));
            
            verts.push_back({.pos = p1 * size + pos, .nor = nor, .uv = {}, .col = col});
            verts.push_back({.pos = p2 * size + pos, .nor = nor, .uv = {}, .col = col});
            verts.push_back({.pos = p3 * size + pos, .nor = nor, .uv = {}, .col = col});
            inds.push_back(i * 3 + 0 + prev_vtx_count);
            inds.push_back(i * 3 + 1 + prev_vtx_count);
            inds.push_back(i * 3 + 2 + prev_vtx_count);
        }
    }
    else {
        for(size_t i = 0; i < ARRSIZE(precalc_positions); ++i) {
            const glm::vec3& p = precalc_positions[i];
            verts.push_back({.pos = p * size + pos, .nor = p, .uv = {}, .col = col});
        }
        for(size_t i = 0; i < ARRSIZE(precalc_indices); ++i) {
            inds.push_back(precalc_indices[i] + prev_vtx_count);
        }
    }
}


static void SetPixel(uint8_t *dest, int destWidth, int x, int y, const uint8_t *color)
{
    uint8_t *pixel = &dest[x * 3 + y * (destWidth * 3)];
    pixel[0] = color[0];
    pixel[1] = color[1];
    pixel[2] = color[2];
}
static void RandomColor(uint8_t *color)
{
	for (int i = 0; i < 3; i++)
		color[i] = uint8_t((rand() % 255 + 192) * 0.5f);
}

// https://github.com/miloyip/line/blob/master/line_bresenham.c
// License: public domain.
static void RasterizeLine(uint8_t *dest, int destWidth, const int *p1, const int *p2, const uint8_t *color)
{
	const int dx = abs(p2[0] - p1[0]), sx = p1[0] < p2[0] ? 1 : -1;
	const int dy = abs(p2[1] - p1[1]), sy = p1[1] < p2[1] ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2;
	int current[2];
	current[0] = p1[0];
	current[1] = p1[1];
	while (SetPixel(dest, destWidth, current[0], current[1], color), current[0] != p2[0] || current[1] != p2[1])
	{
		const int e2 = err;
		if (e2 > -dx) { err -= dy; current[0] += sx; }
		if (e2 < dy) { err += dx; current[1] += sy; }
	}
}

/*
https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
Copyright Dmitry V. Sokolov

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
static void RasterizeTriangle(uint8_t *dest, int destWidth, const int *t0, const int *t1, const int *t2, const uint8_t *color)
{
	if (t0[1] > t1[1]) std::swap(t0, t1);
	if (t0[1] > t2[1]) std::swap(t0, t2);
	if (t1[1] > t2[1]) std::swap(t1, t2);
	int total_height = t2[1] - t0[1];
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1[1] - t0[1] || t1[1] == t0[1];
		int segment_height = second_half ? t2[1] - t1[1] : t1[1] - t0[1];
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1[1] - t0[1] : 0)) / segment_height;
		int A[2], B[2];
		for (int j = 0; j < 2; j++) {
			A[j] = int(t0[j] + (t2[j] - t0[j]) * alpha);
			B[j] = int(second_half ? t1[j] + (t2[j] - t1[j]) * beta : t0[j] + (t1[j] - t0[j]) * beta);
		}
		if (A[0] > B[0]) std::swap(A, B);
		for (int j = A[0]; j <= B[0]; j++)
			SetPixel(dest, destWidth, j, t0[1] + i, color);
	}
}
static void XAtlasDebugRender(const xatlas::Atlas* atlas) {
    if (atlas->width > 0 && atlas->height > 0) {
        printf("Rasterizing result...\n");
        // Dump images.
        std::vector<uint8_t> outputTrisImage, outputChartsImage;
        const uint32_t imageDataSize = atlas->width * atlas->height * 3;
        outputTrisImage.resize(atlas->atlasCount * imageDataSize);
        outputChartsImage.resize(atlas->atlasCount * imageDataSize);
        for (uint32_t i = 0; i < atlas->meshCount; i++) {
            const xatlas::Mesh &mesh = atlas->meshes[i];
            // Rasterize mesh triangles.
            const uint8_t white[] = { 255, 255, 255 };
            for (uint32_t j = 0; j < mesh.indexCount; j += 3) {
                int32_t atlasIndex = -1;
                bool skip = false;
                int verts[3][2];
                for (int k = 0; k < 3; k++) {
                    const xatlas::Vertex &v = mesh.vertexArray[mesh.indexArray[j + k]];
                    if (v.atlasIndex == -1) {
                        skip = true;
                        break;
                    }
                    atlasIndex = v.atlasIndex;
                    verts[k][0] = int(v.uv[0]);
                    verts[k][1] = int(v.uv[1]);
                }
                if (skip)
                    continue; // Skip triangles that weren't atlased.
                uint8_t color[3];
                RandomColor(color);
                uint8_t *imageData = &outputTrisImage[atlasIndex * imageDataSize];
                RasterizeTriangle(imageData, atlas->width, verts[0], verts[1], verts[2], color);
                RasterizeLine(imageData, atlas->width, verts[0], verts[1], white);
                RasterizeLine(imageData, atlas->width, verts[1], verts[2], white);
                RasterizeLine(imageData, atlas->width, verts[2], verts[0], white);
            }
            // Rasterize mesh charts.
            for (uint32_t j = 0; j < mesh.chartCount; j++) {
                const xatlas::Chart *chart = &mesh.chartArray[j];
                uint8_t color[3];
                RandomColor(color);
                for (uint32_t k = 0; k < chart->faceCount; k++) {
                    int verts[3][2];
                    for (int l = 0; l < 3; l++) {
                        const xatlas::Vertex &v = mesh.vertexArray[mesh.indexArray[chart->faceArray[k] * 3 + l]];
                        verts[l][0] = int(v.uv[0]);
                        verts[l][1] = int(v.uv[1]);
                    }
                    uint8_t *imageData = &outputChartsImage[chart->atlasIndex * imageDataSize];
                    RasterizeTriangle(imageData, atlas->width, verts[0], verts[1], verts[2], color);
                    RasterizeLine(imageData, atlas->width, verts[0], verts[1], white);
                    RasterizeLine(imageData, atlas->width, verts[1], verts[2], white);
                    RasterizeLine(imageData, atlas->width, verts[2], verts[0], white);
                }
            }
        }
        //for (uint32_t i = 0; i < atlas->atlasCount; i++) {
        //    char filename[256];
        //    snprintf(filename, sizeof(filename), "example_uvmesh_tris%02u.tga", i);
        //    printf("Writing '%s'...\n", filename);
        //    stbi_write_tga(filename, atlas->width, atlas->height, 3, &outputTrisImage[i * imageDataSize]);
        //    snprintf(filename, sizeof(filename), "example_uvmesh_charts%02u.tga", i);
        //    printf("Writing '%s'...\n", filename);
        //    stbi_write_tga(filename, atlas->width,atlas->height, 3, &outputChartsImage[i * imageDataSize]);
        //}
    }
}

ISize GenerateUVs(std::vector<Vertex>& verts, const std::vector<uint32_t>& inds, uint32_t max_resolution) {
    if(verts.empty()) {
        return {0, 0};
    }
    
    xatlas::MeshDecl mesh_decl;
    mesh_decl.vertexCount = verts.size();
    mesh_decl.vertexPositionData = &verts.data()->pos;
    mesh_decl.vertexNormalData = &verts.data()->nor;
    mesh_decl.vertexNormalStride = sizeof(Vertex);
    mesh_decl.vertexPositionStride = sizeof(Vertex);
    mesh_decl.indexCount = inds.size();
    mesh_decl.indexData = inds.data();
    mesh_decl.indexFormat = xatlas::IndexFormat::UInt32;

    xatlas::Atlas* atlas = xatlas::Create();
    
    xatlas::AddMeshError error = xatlas::AddMesh(atlas, mesh_decl);
    if(error != xatlas::AddMeshError::Success) {
        xatlas::Destroy(atlas);
        std::cout << "[xatlas]: Error adding mesh '" <<  xatlas::StringForEnum(error) << "'" << std::endl;
        return {0, 0};
    }
    xatlas::PackOptions options;
    options.resolution = max_resolution;
    options.padding = 4;
    xatlas::Generate(atlas, xatlas::ChartOptions(), options);
    
    
    if(atlas->atlasCount != 1) {
        std::cout << "[xatlas]: Error xatlas created " << atlas->atlasCount << " atlases, but I just want exactly one in the GenerateUV function" << std::endl;
        xatlas::Destroy(atlas);
        return {0, 0};
    }
    XAtlasDebugRender(atlas);

    glm::vec2 inv_sz = 1.0f / glm::vec2((float)atlas->width, (float)atlas->height);
    for(uint32_t i = 0; i < atlas->meshCount; ++i) {
        const xatlas::Mesh& mesh = atlas->meshes[i];
        for(uint32_t j = 0; j < mesh.vertexCount; ++j) {
            const xatlas::Vertex &v = mesh.vertexArray[j];
            if(v.xref < verts.size()) {
                verts.at(v.xref).uv = (glm::vec2(v.uv[0], v.uv[1]) * inv_sz);
            }
        }
    }
    
    xatlas::Destroy(atlas);
    return {atlas->width, atlas->height};
}
static std::string EXECUTABLE_PATH;
void SetExecutablePath(const char* path) {
    EXECUTABLE_PATH = path;
    const size_t last_backslash = EXECUTABLE_PATH.find_last_of('\\');
    const size_t last_slash = EXECUTABLE_PATH.find_last_of('/');
    if(last_slash != SIZE_MAX && last_backslash != SIZE_MAX) {
        if(last_slash < last_backslash) {
            EXECUTABLE_PATH = EXECUTABLE_PATH.substr(0, last_backslash + 1);
        }
        else {
            EXECUTABLE_PATH = EXECUTABLE_PATH.substr(0, last_slash + 1);
        }
    }
    else if(last_slash != SIZE_MAX) {
        EXECUTABLE_PATH = EXECUTABLE_PATH.substr(0, last_slash + 1);
    }
    else if(last_backslash != SIZE_MAX) {
        EXECUTABLE_PATH = EXECUTABLE_PATH.substr(0, last_backslash + 1);
    }
}
std::string TranslateRelativePath(const std::string& path) {
    return EXECUTABLE_PATH + path;
}

glm::vec4 RgbaToHsla(const glm::vec4& color) {
    float r = color.r, g = color.g, b = color.b;
    float maxVal = std::max({r, g, b});
    float minVal = std::min({r, g, b});
    float delta = maxVal - minVal;

    float hue = 0.0f;
    float saturation = 0.0f;
    float lightness = (maxVal + minVal) / 2.0f;

    if (delta > 0.0f) {
        saturation = (lightness < 0.5f) ? (delta / (maxVal + minVal)) : (delta / (2.0f - maxVal - minVal));

        if (maxVal == r) {
            hue = (g - b) / delta + (g < b ? 6.0f : 0.0f);
        } else if (maxVal == g) {
            hue = (b - r) / delta + 2.0f;
        } else {
            hue = (r - g) / delta + 4.0f;
        }
        hue /= 6.0f;
    }

    return glm::vec4(hue, saturation, lightness, color.a);
}
glm::vec4 HslaToRgba(const glm::vec4& hsl) {
    float h = hsl.x, s = hsl.y, l = hsl.z;

    auto hueToRgb = [](float p, float q, float t) {
        if (t < 0.0f) t += 1.0f;
        if (t > 1.0f) t -= 1.0f;
        if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
        if (t < 1.0f / 2.0f) return q;
        if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
        return p;
    };

    float r, g, b;
    if (s == 0.0f) {
        r = g = b = l; // Achromatic
    } else {
        float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
        float p = 2.0f * l - q;
        r = hueToRgb(p, q, h + 1.0f / 3.0f);
        g = hueToRgb(p, q, h);
        b = hueToRgb(p, q, h - 1.0f / 3.0f);
    }
    return glm::vec4(r, g, b, hsl.a);
}


