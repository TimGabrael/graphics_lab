#pragma once
#include "util.h"


struct BasicShader {
    BasicShader();
    ~BasicShader();
    void Bind() const;
    void SetModelMatrix(const glm::mat4& mat) const;
    void SetViewMatrix(const glm::mat4& mat) const;
    void SetProjectionMatrix(const glm::mat4& mat) const;

    GLuint program;
    GLint model_loc;
    GLint view_loc;
    GLint proj_loc;
};



