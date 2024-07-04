#include "shaders.h"

BasicShader::BasicShader() {
    this->program = LoadProgramFromFile("../assets/shaders/basic.vs", "../assets/shaders/basic.fs");
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
