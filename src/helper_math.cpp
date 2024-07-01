#include "helper_math.h"
#include <glm/ext/scalar_constants.hpp>
#include <random>
#include <corecrt_math_defines.h>


float CalcTriangleArea(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
    const glm::vec3 v12 = p2 - p1;
    const glm::vec3 v13 = p3 - p1;
    
    const float s1 = v12.y * v13.z - v12.z * v13.y;
    const float s2 = v12.z * v13.x - v12.x * v13.z;
    const float s3 = v12.x * v13.y - v12.y * v13.x;
    return 0.5f * sqrtf(s1 * s1 + s2 * s2 + s3 * s3);
}

float AngleBetweenVectors(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& reference_up) {
    float dot = glm::dot(v1, v2);

    float mag1 = glm::length(v1);
    float mag2 = glm::length(v2);

    float cosTheta = dot / (mag1 * mag2);

    if (cosTheta > 1.0f) cosTheta = 1.0f;
    if (cosTheta < -1.0f) cosTheta = -1.0f;

    float angle = glm::acos(cosTheta);

    glm::vec3 cross = glm::cross(v1, v2);

    if (glm::dot(cross, reference_up) < 0.0f) {
        angle = 2.0f * glm::pi<float>() - angle;
    }
    return angle;
}

uint32_t ConvertColor(const glm::vec4& col) {
    const glm::vec4 clipped = glm::min(glm::max(col, {0.0f, 0.0f, 0.0f, 0.0f}), {1.0f, 1.0f, 1.0f, 1.0f});
    const uint32_t out_col = ((uint32_t)(255 * clipped.a) << 24) | ((uint32_t)(255 * clipped.b) << 16) | ((uint32_t)(255 * clipped.g) << 8) | ((uint32_t)(255 * clipped.r));
    return out_col;
}
float GetRandomFloat(float start, float end) {
    static std::random_device device;
    static std::mt19937 ran(device());
    std::uniform_real_distribution<float> dist(start, end);
    return dist(ran);
}
glm::vec3 GetRandomNormalizedVector() {
    static std::random_device device;
    static std::mt19937 ran(device());
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    glm::vec3 dir = glm::vec3(dist(ran), dist(ran), dist(ran));
    return glm::normalize(dir);
}

glm::vec2 GetRandomPointInSquare() {
    return glm::vec2(GetRandomFloat(-0.5f, 0.5f), GetRandomFloat(-0.5f, 0.5f));
}
glm::vec2 GetRandomPointInCircle() {
    const float angle = GetRandomFloat(0.0f, 2.0f * M_PI);
    return glm::vec2(cosf(angle), sinf(angle)) * sqrtf(GetRandomFloat(0.0f, 1.0f));
}
