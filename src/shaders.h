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
struct BasicShaderInstanced {
    struct InstanceData {
        glm::vec3 pos;
        glm::vec4 col;
    };
    BasicShaderInstanced();
    ~BasicShaderInstanced();
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
        float iTime = 0.0f;
        int iFrame = 0;
        float padding_1 = 0.0f;
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

struct CascadedShadowShader {
    CascadedShadowShader();
    ~CascadedShadowShader();

    void Bind() const;
    void SetModelMatrix(const glm::mat4& mat) const;
    void SetViewMatrix(const glm::mat4& mat) const;
    void SetProjMatrix(const glm::mat4& mat) const;
    void SetLightDirection(const glm::vec3& light_dir) const;
    void SetLightProjAndBounds(const glm::mat4 projections[4], const glm::vec4 bounds[4]) const;
    void SetDebugView(bool on) const;

    void SetColorTexture(GLuint id) const;
    void SetShadowMap(GLuint id) const;

    GLuint program;
    GLuint model_loc;
    GLuint view_loc;
    GLuint proj_loc;

    GLuint light_direction_loc;
    GLuint light_proj_loc;
    GLuint light_bounds_loc;
    GLuint enable_debug_view_loc;
};
struct DepthOnlyShader {
    DepthOnlyShader();
    ~DepthOnlyShader();

    void Bind() const;
    void SetViewProjMatrix(const glm::mat4& view_proj) const;
    void SetModelMatrix(const glm::mat4& model) const;

    GLuint program;
    GLuint model_loc;
    GLuint view_proj_loc;
};
struct ColorShader {
    ColorShader();
    ~ColorShader();

    void Bind();
    void SetViewProjMatrix(const glm::mat4& view_proj);
    void SetModelMatrix(const glm::mat4& model);
    void SetColor(const glm::vec4& col);

    GLuint program;
    GLuint model_loc;
    GLuint view_proj_loc;
    GLuint color_loc;
};
struct LightRayShader {
    LightRayShader();
    ~LightRayShader();

    void Bind();
    void SetDensity(float density);
    void SetWeight(float weight);
    void SetDecay(float decay);
    void SetExposure(float exposure);
    void SetNumSamples(uint32_t num_samples);
    void SetLightScreenSpacePos(const glm::vec2& light_pos);
    void SetOcclusionTexture(GLuint occlusion_texture);

    
    GLuint program;
    GLuint density_loc;
    GLuint weight_loc;
    GLuint decay_loc;
    GLuint exposure_loc;
    GLuint num_samples_loc;
    GLuint light_pos_loc;
};


