#include "shaders.h"
#include <iostream>

BasicShader::BasicShader() {
    this->program = LoadProgramFromFile("../../assets/shaders/basic.vs", "../../assets/shaders/basic.fs");
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
    this->program = LoadProgramFromFile("../../assets/shaders/basic.vs", "../../assets/shaders/intersection.fs");
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
