cmake_minimum_required(VERSION 3.28)
project(lab)


find_package(glm REQUIRED)
find_package(glad REQUIRED)
find_package(glfw3 REQUIRED)

set(IMGUI_SRC "ImGui/imgui.cpp" "ImGui/imgui_demo.cpp" "ImGui/imgui_draw.cpp" "ImGui/imgui_impl_glfw.cpp" "ImGui/imgui_impl_opengl3.cpp" "ImGui/imgui_tables.cpp" "ImGui/imgui_widgets.cpp")
set(XATLAS_SRC "xatlas/xatlas.cpp")

add_executable(graph src/main.cpp src/util.cpp src/helper_math.cpp src/mesh_decimation.cpp src/raytracer.cpp ${IMGUI_SRC} ${XATLAS_SRC})
target_include_directories(graph PRIVATE ImGui xatlas)

target_link_libraries(graph PRIVATE glm::glm glad::glad glfw)

