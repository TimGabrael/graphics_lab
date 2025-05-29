cmake_minimum_required(VERSION 3.25)
project(lab)

set(CMAKE_CXX_STANDARD 17)

find_package(glm REQUIRED)
find_package(glad REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Freetype REQUIRED)

set(IMGUI_SRC "ImGui/imgui.cpp" "ImGui/imgui_demo.cpp" "ImGui/imgui_draw.cpp" "ImGui/imgui_impl_glfw.cpp" "ImGui/imgui_impl_opengl3.cpp" "ImGui/imgui_tables.cpp" "ImGui/imgui_widgets.cpp")
set(XATLAS_SRC "xatlas/xatlas.cpp")

add_executable(graph src/main.cpp src/util.cpp src/helper_math.cpp src/mesh_decimation.cpp src/raytracer.cpp ${IMGUI_SRC} ${XATLAS_SRC} src/shaders.cpp src/FastNoise.cpp src/atlas.cpp)
target_include_directories(graph PRIVATE ImGui xatlas)

target_link_libraries(graph PRIVATE glm::glm glad::glad glfw Freetype::Freetype)


