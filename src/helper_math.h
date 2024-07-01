#pragma once
#include <glm/glm.hpp>


float CalcTriangleArea(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);

float AngleBetweenVectors(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& reference_up);


uint32_t ConvertColor(const glm::vec4& col);

// [start, end]
float GetRandomFloat(float start, float end);
glm::vec3 GetRandomNormalizedVector();


glm::vec2 GetRandomPointInSquare();
glm::vec2 GetRandomPointInCircle();



