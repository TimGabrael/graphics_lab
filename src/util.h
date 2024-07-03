#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vector>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "tiny_gltf.h"
#include <corecrt_math_defines.h>

#define INVALID_GL_HANDLE 0xFFFFFFFFu
#define ARRSIZE(arr) (sizeof(arr) / sizeof(*arr))

struct Triangle {
    glm::vec3 p1;
    glm::vec3 p2;
    glm::vec3 p3;
};
struct ISize {
    uint32_t width;
    uint32_t height;
};

struct Texture2D {
    Texture2D(){}
    Texture2D(Texture2D&&);
    ~Texture2D();
    Texture2D& operator=(Texture2D&& o);
    Texture2D(const Texture2D&) = delete;
    enum DataType {
        Float,
        Uint,
    };
    GLuint id = INVALID_GL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t num_channels = 0;
    void* data = nullptr;
    DataType type;
};


struct Vertex {
    glm::vec3 pos;
    glm::vec3 nor;
    glm::vec2 uv;
    glm::vec4 col;
};
struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};

struct SimpleMaterial {
    Texture2D* diffuse_texture;
};

struct Material {
    enum Type {
        SIMPLE_MATERIAL,
    };
    union {
        SimpleMaterial simple;
    };
    Type type;
};
struct Mesh {
    Mesh() {}
    ~Mesh();
    Mesh(Mesh&&);
    Mesh(const Mesh&) = delete;
    Mesh& operator=(Mesh&&);

    GLuint vao = INVALID_GL_HANDLE;
    GLuint vbo = INVALID_GL_HANDLE;
    GLuint ibo = INVALID_GL_HANDLE;
    Vertex* verts = nullptr;
    uint32_t* inds = nullptr;
    uint32_t vertex_count;
    uint32_t triangle_count;
    BoundingBox bb;
    uint32_t material_idx;
};


struct GLTFLoadData {
    GLTFLoadData(GLTFLoadData&&) = default;
    GLTFLoadData(GLTFLoadData&) = delete;
    GLTFLoadData& operator=(GLTFLoadData&&) = default;

    struct TextureTranform {
        glm::vec2 offset = {0.0f, 0.0f};
        glm::vec2 scale = {1.0f, 1.0f};
    };
    struct TextureInfo {
        uint32_t idx = -1;
        uint32_t tex_coord_idx = 0;
        TextureTranform transform;
    };
    struct Material {
        TextureInfo base_color;
    };

    struct GltfMesh {
        Mesh mesh;
        uint32_t material_idx;
    };

    std::vector<GltfMesh> meshes;
    std::vector<Texture2D> textures;
    std::vector<GLTFLoadData::Material> materials;
};


Mesh CreateMesh(const Vertex* verts, const uint32_t* inds, uint32_t vert_count, uint32_t ind_count);
void DestroyMesh(Mesh* mesh);
GLTFLoadData LoadGLTFLoadData(const char* filename);
void DestroyGLTFLoadData(GLTFLoadData* load);

Texture2D CreateTexture2D(const uint32_t* data, uint32_t width, uint32_t height);
Texture2D CreateTexture2D(const glm::vec4* data, uint32_t width, uint32_t height);
void DestroyTexture2D(Texture2D* tex);


GLuint LoadProgram(const char* vs, const char* fs);
GLuint LoadProgramFromFile(const char* vs, const char* fs);

void DrawHDR(const Texture2D& tex, const glm::mat4& view_matrix);

void GenerateCube(std::vector<Vertex>& verts, std::vector<uint32_t>& inds);
void GenerateCube(std::vector<Vertex>& verts, std::vector<uint32_t>& inds, const glm::vec3& size, const glm::vec4 col[8]);

ISize GenerateUVs(std::vector<Vertex>& verts, const std::vector<uint32_t>& inds);





