#include "shaders.h"
#include <iostream>
#include <chrono>
#include <thread>

BasicShader::BasicShader() {
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/basic.vs").c_str(), TranslateRelativePath("../../assets/shaders/basic.fs").c_str());
    this->model_loc = glGetUniformLocation(this->program, "model");
    this->view_loc = glGetUniformLocation(this->program, "view");
    this->proj_loc = glGetUniformLocation(this->program, "projection");
}
BasicShader::~BasicShader() {
    glDeleteProgram(this->program);
}
void BasicShader::Bind() const {
    glUseProgram(this->program);
}
void BasicShader::SetModelMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->model_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void BasicShader::SetViewMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->view_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void BasicShader::SetProjectionMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->proj_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void BasicShader::SetTexture(GLuint id) const {
    glBindTexture(GL_TEXTURE_2D, id);
}
BasicShaderInstanced::BasicShaderInstanced() {
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/basic_instanced.vs").c_str(), TranslateRelativePath("../../assets/shaders/basic.fs").c_str());
    this->model_loc = glGetUniformLocation(this->program, "model");
    this->view_loc = glGetUniformLocation(this->program, "view");
    this->proj_loc = glGetUniformLocation(this->program, "projection");
}
BasicShaderInstanced::~BasicShaderInstanced() {
    glDeleteProgram(this->program);
}
void BasicShaderInstanced::Bind() const {
    glUseProgram(this->program);
}
void BasicShaderInstanced::SetModelMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->model_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void BasicShaderInstanced::SetViewMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->view_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void BasicShaderInstanced::SetProjectionMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->proj_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void BasicShaderInstanced::SetTexture(GLuint id) const {
    glBindTexture(GL_TEXTURE_2D, id);
}

IntersectionHightlightingShader::IntersectionHightlightingShader() {
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/basic.vs").c_str(), TranslateRelativePath("../../assets/shaders/intersection.fs").c_str());
    glUseProgram(this->program);
    this->model_loc = glGetUniformLocation(this->program, "model");
    this->view_loc = glGetUniformLocation(this->program, "view");
    this->proj_loc = glGetUniformLocation(this->program, "projection");
    this->screen_size_loc = glGetUniformLocation(this->program, "screenSize");
    uint32_t data_idx = glGetUniformBlockIndex(this->program, "Data");
    glUniformBlockBinding(this->program, data_idx, DATA_BIND_LOC);
    glGenBuffers(1, &this->uniform_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
    Data default_data = {};

    glBufferData(GL_UNIFORM_BUFFER, sizeof(Data), &default_data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    GLuint col_loc = glGetUniformLocation(this->program, "noise_map");
    GLuint depth_loc = glGetUniformLocation(this->program, "depth_map");
    glUniform1i(col_loc, 0);
    glUniform1i(depth_loc, 1);
}
IntersectionHightlightingShader::~IntersectionHightlightingShader() {
    glDeleteProgram(this->program);
    glDeleteBuffers(1, &this->uniform_buffer);
}
void IntersectionHightlightingShader::Bind() const {
    glUseProgram(this->program);
    glBindBufferBase(GL_UNIFORM_BUFFER, DATA_BIND_LOC, this->uniform_buffer);
}
void IntersectionHightlightingShader::SetModelMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->model_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void IntersectionHightlightingShader::SetViewMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->view_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void IntersectionHightlightingShader::SetProjectionMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->proj_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void IntersectionHightlightingShader::SetScreenSize(float width, float height) const {
    glUniform2f(this->screen_size_loc, width, height);
}
void IntersectionHightlightingShader::SetData(const Data& data) const {
    glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Data), &data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void IntersectionHightlightingShader::SetColorTexture(GLuint id) const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);
}
void IntersectionHightlightingShader::SetDepthTexture(GLuint id) const {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, id);
}
TriangleVisibilityShader::TriangleVisibilityShader() {
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/triangle_visibility.vs").c_str(), TranslateRelativePath("../../assets/shaders/triangle_visibility.fs").c_str());
    glUseProgram(this->program);
    this->model_loc = glGetUniformLocation(this->program, "model");
    this->view_loc = glGetUniformLocation(this->program, "view");
    this->proj_loc = glGetUniformLocation(this->program, "projection");
}
TriangleVisibilityShader::~TriangleVisibilityShader() {
    glDeleteProgram(this->program);
}
void TriangleVisibilityShader::Bind() const {
    glUseProgram(this->program);
}
void TriangleVisibilityShader::SetModelMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->model_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void TriangleVisibilityShader::SetViewMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->view_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void TriangleVisibilityShader::SetProjectionMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->proj_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
VolumetricFogShader::VolumetricFogShader() {
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/volumetric_fog.vs").c_str(), TranslateRelativePath("../../assets/shaders/volumetric_fog.fs").c_str());
    glUseProgram(this->program);
    this->inv_view_loc = glGetUniformLocation(this->program, "invViewMatrix");
    this->inv_proj_loc = glGetUniformLocation(this->program, "invProjMatrix");
    this->screen_size_loc = glGetUniformLocation(this->program, "screen_size");

    uint32_t data_idx = glGetUniformBlockIndex(this->program, "FogData");
    glUniformBlockBinding(this->program, data_idx, 0);
    glGenBuffers(1, &this->uniform_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
    FogData default_data = {};
    default_data.color = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
    default_data.center_and_radius = glm::vec4(0.0f, 0.0f, 0.0f, 4.0f);

    glBufferData(GL_UNIFORM_BUFFER, sizeof(FogData), &default_data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    static glm::vec2 positions[] = {
        {-1.0f,  1.0f},
        {-1.0f, -1.0f},
        { 1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
    };

    glCreateVertexArrays(1, &this->vao);
    glBindVertexArray(this->vao);
    glGenBuffers(1, &this->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * ARRSIZE(positions), positions, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
    glBindVertexArray(0);
}
VolumetricFogShader::~VolumetricFogShader() {
    if(this->uniform_buffer != GL_INVALID_VALUE) {
        glDeleteBuffers(1, &this->uniform_buffer);
        this->uniform_buffer = GL_INVALID_VALUE;
    }
    if(this->vbo != GL_INVALID_VALUE) {
        glDeleteBuffers(1, &this->vbo);
        this->vbo = GL_INVALID_VALUE;
    }
    if(this->program != GL_INVALID_VALUE) {
        glDeleteProgram(this->program);
        this->program = GL_INVALID_VALUE;
    }
    if(this->vao != GL_INVALID_VALUE) {
        glDeleteVertexArrays(1, &this->vao);
        this->vao = GL_INVALID_VALUE;
    }
}
void VolumetricFogShader::Draw() const {
    this->Bind();
    glBindVertexArray(this->vao);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);

}
void VolumetricFogShader::Bind() const {
    glUseProgram(this->program);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->uniform_buffer);
}
void VolumetricFogShader::SetViewMatrix(const glm::mat4& mat) const {
    glm::mat4 view_mat = glm::inverse(mat);
    glUniformMatrix4fv(this->inv_view_loc, 1, GL_FALSE, (const GLfloat*)&view_mat);
}
void VolumetricFogShader::SetProjMat(const glm::mat4& mat) const {
    glm::mat4 proj_mat = glm::inverse(mat);
    glUniformMatrix4fv(this->inv_proj_loc, 1, GL_FALSE, (const GLfloat*)&proj_mat);
}
void VolumetricFogShader::SetScreenSize(const glm::vec2& win_size) const {
    glUniform2fv(this->screen_size_loc, 1, (const GLfloat*)&win_size);
}
void VolumetricFogShader::SetData(const FogData& data) const {
    glBindBuffer(GL_UNIFORM_BUFFER, this->uniform_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(FogData), &data, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

CascadedShadowShader::CascadedShadowShader() {
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/cascaded_shadow_map.vs").c_str(), TranslateRelativePath("../../assets/shaders/cascaded_shadow_map.fs").c_str());
    glUseProgram(this->program);
    this->view_loc = glGetUniformLocation(this->program, "view");
    this->proj_loc = glGetUniformLocation(this->program, "projection");
    this->model_loc = glGetUniformLocation(this->program, "model");
    this->light_direction_loc = glGetUniformLocation(this->program, "light_direction");
    this->enable_debug_view_loc = glGetUniformLocation(this->program, "enable_debug_view");
    this->light_proj_loc = glGetUniformLocation(this->program, "light_projections");
    this->light_bounds_loc = glGetUniformLocation(this->program, "light_projection_bounds");


    GLuint col_loc = glGetUniformLocation(this->program, "color_map");
    GLuint shadow_loc = glGetUniformLocation(this->program, "shadow_map");
    glUniform1i(col_loc, 0);
    glUniform1i(shadow_loc, 1);
}
CascadedShadowShader::~CascadedShadowShader() {
    glDeleteProgram(this->program);
    this->program = INVALID_GL_HANDLE;
}

void CascadedShadowShader::Bind() const {
    glUseProgram(this->program);
}
void CascadedShadowShader::SetModelMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->model_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void CascadedShadowShader::SetViewMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->view_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void CascadedShadowShader::SetProjMatrix(const glm::mat4& mat) const {
    glUniformMatrix4fv(this->proj_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void CascadedShadowShader::SetLightDirection(const glm::vec3& light_dir) const {
    glUniform3f(this->light_direction_loc, light_dir.x, light_dir.y, light_dir.z);
}
void CascadedShadowShader::SetLightProjAndBounds(const glm::mat4 projections[4], const glm::vec4 bounds[4]) const {
    glUniformMatrix4fv(this->light_proj_loc, 4, GL_FALSE, (const GLfloat*)projections);
    glUniform4fv(this->light_bounds_loc, 4, (const GLfloat*)bounds);
}
void CascadedShadowShader::SetDebugView(bool on) const {
    glUniform1i(this->enable_debug_view_loc, on);
}
void CascadedShadowShader::SetColorTexture(GLuint id) const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);
}
void CascadedShadowShader::SetShadowMap(GLuint id) const {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, id);
}

DepthOnlyShader::DepthOnlyShader() {
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/depth_only.vs").c_str(), TranslateRelativePath("../../assets/shaders/depth_only.fs").c_str());
    glUseProgram(this->program);
    this->view_proj_loc = glGetUniformLocation(this->program, "view_proj");
    this->model_loc = glGetUniformLocation(this->program, "model");
}
DepthOnlyShader::~DepthOnlyShader() {
    glDeleteProgram(this->program);
}
void DepthOnlyShader::Bind() const {
    glUseProgram(this->program);
}
void DepthOnlyShader::SetViewProjMatrix(const glm::mat4& view_proj) const {
    glUniformMatrix4fv(this->view_proj_loc, 1, GL_FALSE, (const GLfloat*)&view_proj);
}
void DepthOnlyShader::SetModelMatrix(const glm::mat4& model) const {
    glUniformMatrix4fv(this->model_loc, 1, GL_FALSE, (const GLfloat*)&model);
}

ColorShader::ColorShader() {
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/color_shader.vs").c_str(), TranslateRelativePath("../../assets/shaders/color_shader.fs").c_str());
    glUseProgram(this->program);
    this->view_proj_loc = glGetUniformLocation(this->program, "view_proj");
    this->model_loc = glGetUniformLocation(this->program, "model");
    // this doesn't work for whatever reason
    this->color_loc = glGetUniformLocation(this->program, "basic_color");
    if(this->color_loc == -1) {
        std::cout << "color_loc is invalid" << std::endl;
    }
}
ColorShader::~ColorShader() {
    glDeleteProgram(this->program);
}
void ColorShader::Bind() {
    glUseProgram(this->program);
}
void ColorShader::SetViewProjMatrix(const glm::mat4& view_proj) {
    glUniformMatrix4fv(this->view_proj_loc, 1, GL_FALSE, (const GLfloat*)&view_proj);
}
void ColorShader::SetModelMatrix(const glm::mat4& model) {
    glUniformMatrix4fv(this->model_loc, 1, GL_FALSE, (const GLfloat*)&model);
}
void ColorShader::SetColor(const glm::vec4& col) {
    glUniform4fv(this->color_loc, 1, (const GLfloat*)&col);
}

LightRayShader::LightRayShader() {
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/lightray_shader.vs").c_str(), TranslateRelativePath("../../assets/shaders/lightray_shader.fs").c_str());
    glUseProgram(this->program);
    this->light_pos_loc = glGetUniformLocation(this->program, "uScreenSpaceSunPos");
    this->density_loc = glGetUniformLocation(this->program, "uDensity");
    this->weight_loc = glGetUniformLocation(this->program, "uWeight");
    this->decay_loc = glGetUniformLocation(this->program, "uDecay");
    this->exposure_loc = glGetUniformLocation(this->program, "uExposure");
    this->num_samples_loc = glGetUniformLocation(this->program, "uNumSamples");
}
LightRayShader::~LightRayShader() {
    glDeleteProgram(this->program);
}
void LightRayShader::Bind() {
    glUseProgram(this->program);
}
void LightRayShader::SetDensity(float density) {
    glUniform1fv(this->density_loc, 1, (const GLfloat*)&density);
}
void LightRayShader::SetWeight(float weight) {
    glUniform1fv(this->weight_loc, 1, (const GLfloat*)&weight);
}
void LightRayShader::SetDecay(float decay) {
    glUniform1fv(this->decay_loc, 1, (const GLfloat*)&decay);
}
void LightRayShader::SetExposure(float exposure) {
    glUniform1fv(this->exposure_loc, 1, (const GLfloat*)&exposure);
}
void LightRayShader::SetNumSamples(uint32_t num_samples) {
    glUniform1iv(this->num_samples_loc, 1, (const GLint*)&num_samples);
}
void LightRayShader::SetLightScreenSpacePos(const glm::vec2& light_pos) {
    glUniform2fv(this->light_pos_loc, 1, (const GLfloat*)&light_pos);
}
void LightRayShader::SetOcclusionTexture(GLuint occlusion_texture) {
    glBindTexture(GL_TEXTURE_2D, occlusion_texture);
}

//layout(binding = 0) uniform sampler2D face_texture;  // 2D cubemap face
//layout(binding = 1, std430) buffer SHBuffer {
//    vec3 sh_coeffs[9]; // SH L2 (9 bands)
//};
//
//uniform mat4 inv_view_proj; // Converts pixel (x,y) to direction
//uniform int image_width;
//uniform int image_height;

SphericalHarmonicsComputeShader::SphericalHarmonicsComputeShader(){
    this->program = LoadComputeProgramFromFile(TranslateRelativePath("../../assets/shaders/spherical_harmonics.cs").c_str());
    this->tex_height_loc = glGetUniformLocation(this->program, "image_height");
    this->tex_width_loc = glGetUniformLocation(this->program, "image_width");
    this->mat_loc = glGetUniformLocation(this->program, "inv_view_proj");

    GLuint temp_tex_loc = glGetUniformLocation(this->program, "face_texture");
    glUniform1i(temp_tex_loc, 0);
    this->tex_loc = 0;
}
SphericalHarmonicsComputeShader::~SphericalHarmonicsComputeShader(){
    glDeleteProgram(this->program);
}
void SphericalHarmonicsComputeShader::Draw(uint32_t tex_width, uint32_t tex_height) {
    glDispatchCompute((tex_width+15)/16, (tex_height+15)/16, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}
void SphericalHarmonicsComputeShader::Bind(){
    glUseProgram(this->program);
}
void SphericalHarmonicsComputeShader::SetTexture(GLuint tex, uint32_t width, uint32_t height){
    glUniform1i(this->tex_width_loc, width);
    glUniform1i(this->tex_height_loc, height);
    glActiveTexture(GL_TEXTURE0 + this->tex_loc);
    glBindTexture(GL_TEXTURE_2D, tex);
}
void SphericalHarmonicsComputeShader::SetInvViewProj(const glm::mat4& mat){
    glUniformMatrix4fv(this->mat_loc, 1, false, (const GLfloat*)&mat);
}
void SphericalHarmonicsComputeShader::SetSHBuffer(GLuint buf){
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf);
}
GLuint SphericalHarmonicsComputeShader::CreateSHBuffer(){
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * 9, nullptr, GL_STATIC_DRAW);
    glm::vec4 zeroData[9] = {};
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * 9, zeroData);
    return buffer;
}
void SphericalHarmonicsComputeShader::ClearSHBuffer(GLuint buf) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf);
    glm::vec4 zeroData[9] = {};
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * 9, zeroData);
}
void SphericalHarmonicsComputeShader::DestroySHBuffer(GLuint buf) {
    glDeleteBuffers(1, &buf);
}

SphericalHarmonicsShader::SphericalHarmonicsShader(){
    this->program = LoadProgramFromFile(TranslateRelativePath("../../assets/shaders/basic.vs").c_str(), TranslateRelativePath("../../assets/shaders/spherical_harmonics.fs").c_str());
    this->model_loc = glGetUniformLocation(this->program, "model");
    this->view_loc = glGetUniformLocation(this->program, "view");
    this->proj_loc = glGetUniformLocation(this->program, "projection");
}
SphericalHarmonicsShader::~SphericalHarmonicsShader(){
    glDeleteProgram(this->program);
}
void SphericalHarmonicsShader::Bind() const{
    glUseProgram(this->program);
}
void SphericalHarmonicsShader::SetModelMatrix(const glm::mat4& mat) const{
    glUniformMatrix4fv(this->model_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void SphericalHarmonicsShader::SetViewMatrix(const glm::mat4& mat) const{
    glUniformMatrix4fv(this->view_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void SphericalHarmonicsShader::SetProjectionMatrix(const glm::mat4& mat) const{
    glUniformMatrix4fv(this->proj_loc, 1, GL_FALSE, (const GLfloat*)&mat);
}
void SphericalHarmonicsShader::SetSHBuffer(GLuint buf){
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf);
}
void SphericalHarmonicsShader::SetTexture(GLuint id) const{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id);
}
