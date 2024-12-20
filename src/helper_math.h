#pragma once
#include <glm/glm.hpp>


bool IsPointInTriangle(const glm::vec2& pt, const glm::vec2& t1, const glm::vec2& t2, const glm::vec2& t3);

float CalcTriangleArea(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);

// used for rasterization
float CalcSignedTriangleArea(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3);

float AngleBetweenVectors(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& reference_up);


uint32_t ConvertColor(const glm::vec4& col);

// [start, end]
float GetRandomFloat(float start, float end);
// [start, end]
uint32_t GetRandomUint32(uint32_t start, uint32_t end);
glm::vec3 GetRandomNormalizedVector();


glm::vec2 GetRandomPointInSquare();
glm::vec2 GetRandomPointInCircle();



