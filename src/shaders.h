#pragma once
#include "util.h"


struct BasicShader {
    BasicShader();
    ~BasicShader();
    void Bind() const;
    void SetModelMatrix(const glm::mat4& mat) const;
    void SetViewMatrix(const glm::mat4& mat) const;
    void SetProjectionMatrix(const glm::mat4& mat) const;

    void SetTexture(GLuint id) const;

    GLuint program;
    GLint model_loc;
    GLint view_loc;
    GLint proj_loc;
};

struct IntersectionHightlightingShader {
    struct Data {
        glm::vec4 intersection_color = {1.0f, 0.0f, 1.0f, 0.6f};
        glm::vec3 center = {0.0f, 0.0f, 0.0f};
        float max_extent = 4.0f;
        float radius_start = 0.5f;
        float circle_thickness = 0.1f;
        float epsilon = 0.2f;
        float timer = 0.0f;
    };
    static constexpr GLuint DATA_BIND_LOC = 0;

    IntersectionHightlightingShader();
    ~IntersectionHightlightingShader();
    void Bind() const;
    void SetModelMatrix(const glm::mat4& mat) const;
    void SetViewMatrix(const glm::mat4& mat) const;
    void SetProjectionMatrix(const glm::mat4& mat) const;
    void SetScreenSize(float width, float height) const;

    void SetData(const Data& data) const;

    void SetColorTexture(GLuint id) const;
    void SetDepthTexture(GLuint id) const;



    GLuint program;
    GLuint uniform_buffer;
    GLint model_loc;
    GLint view_loc;
    GLint proj_loc;
    GLint screen_size_loc;
};

struct TriangleVisibilityShader {
    TriangleVisibilityShader();
    ~TriangleVisibilityShader();
    void Bind() const;
    void SetModelMatrix(const glm::mat4& mat) const;
    void SetViewMatrix(const glm::mat4& mat) const;
    void SetProjectionMatrix(const glm::mat4& mat) const;

    GLuint program;
    GLint model_loc;
    GLint view_loc;
    GLint proj_loc;
};

struct VolumetricFogShader {
    struct FogData {
        glm::vec4 center_and_radius; // glm::vec3 center, float radius (w)
        glm::vec4 color;
    };
    VolumetricFogShader();
    ~VolumetricFogShader();

    void Draw() const;
    void Bind() const;
    void SetViewMatrix(const glm::mat4& mat) const;
    void SetProjMat(const glm::mat4& mat) const;
    void SetScreenSize(const glm::vec2& win_size) const;
    void SetData(const FogData& data) const;

    GLuint vao;
    GLuint vbo;

    GLuint program;
    GLuint uniform_buffer;

    GLint inv_view_loc;
    GLint inv_proj_loc;
    GLint screen_size_loc;
};

