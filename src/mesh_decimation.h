#pragma once
#include "util.h"

glm::vec2 GetMeshMinMaxTrianglesSizes(const Mesh* input);

Mesh DecimateMesh(const Mesh* input, float min_triangle_size);



