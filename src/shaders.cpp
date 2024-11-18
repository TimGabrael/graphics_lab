#include "shaders.h"
#include <iostream>

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
    glDeleteBuffers(1, &this->uniform_buffer);
    glDeleteBuffers(1, &this->vbo);
    glDeleteProgram(this->program);
    glDeleteVertexArrays(1, &this->vao);
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

