#include <iostream>
#include "util.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "mesh_decimation.h"
#include "raytracer.h"
#include "stb_image.h"



struct CubeScene {
    CubeScene() {
        std::vector<Vertex> verts;
        std::vector<uint32_t> inds;
        const glm::vec4 cols[6] = {
            {1.0f, 0.0f, 1.0f, 1.0f},
            {0.0f, 1.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.0f, 1.0f, 1.0f, 1.0f},


            //{1.0f, 1.0f, 1.0f, 1.0f},
            //{1.0f, 1.0f, 1.0f, 1.0f},
            //{1.0f, 1.0f, 1.0f, 1.0f},
            //{1.0f, 1.0f, 1.0f, 1.0f},
            //{1.0f, 1.0f, 1.0f, 1.0f},
            //{1.0f, 1.0f, 1.0f, 1.0f},

        };
        GenerateCube(verts, inds, {1.0f, 1.0f, 1.0f}, cols);
        ISize sz = GenerateUVs(verts, inds);
        sz.width /= 5;
        sz.height /= 5;
        this->post_processing = RayImage(sz.width, sz.height);
        ISize small_sz = sz;
        small_sz.width /= 10;
        small_sz.height /= 10;
        int x,y,c;
        stbi_uc* data = stbi_load("example_uvmesh_tris00.tga", &x, &y, &c, 4);
        if(x > 0 && y > 0 && data) {
            this->debug_texture = CreateTexture2D((const uint32_t*)data, x, y);
            stbi_image_free(data);
        }
        else {
            std::cout << "failed to load debug_texture" << std::endl;
        }
        float* hdr_data = stbi_loadf("../assets/garden.hdr", &x, &y, &c, 4);
        if(hdr_data) {
            this->hdr_map = CreateTexture2D((const glm::vec4*)hdr_data, x, y);
            stbi_image_free(hdr_data);
        }


        this->cube_mesh = CreateMesh(verts.data(), inds.data(), verts.size(), inds.size());
        cube_bvh.CreateFromMesh(&cube_mesh, 8);
        RayObject obj = {};
        obj.bvh = &this->cube_bvh;
        this->image = RayImage(this->width, this->height);

        // create the scene
        glm::mat4 mat = glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec3(0.0f, 0.5f, -3.0f));
        obj.mat = mat;
        obj.inv_model_mat = glm::inverse(mat);
        obj.material.specular_col = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        obj.material.emission_strength = 0.0f;
        obj.material.smoothness = 0.0f;
        obj.material.specular_probability = 0.0f;
        this->ray_scene.objects.emplace_back(std::move(obj));
        this->lit_objects.push_back({RayImage(small_sz.width, small_sz.height), 0});


        mat = glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec3(0.0f, 0.5f, 10.0f));
        obj.mat = mat;
        obj.inv_model_mat = glm::inverse(mat);
        obj.material.specular_col = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        obj.material.emission_col = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        obj.material.emission_strength = 10.0f;
        obj.material.smoothness = 0.0f;
        obj.material.specular_probability = 0.0f;
        this->ray_scene.objects.emplace_back(std::move(obj));
        this->lit_objects.push_back({RayImage(small_sz.width, small_sz.height), 1});


        mat = glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(30.0f, 0.1f, 30.0f)), glm::vec3(0.0f, -0.05f, 0.0f));
        obj.mat = mat;
        obj.inv_model_mat = glm::inverse(mat);
        obj.material.specular_col = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        obj.material.emission_col = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        obj.material.emission_strength = 0.0f;
        obj.material.smoothness = 0.0f;
        obj.material.specular_probability = 0.0f;
        this->ray_scene.objects.emplace_back(std::move(obj));
        this->lit_objects.push_back({RayImage(sz.width, sz.height), 2});

        this->ray_scene.hdr_map = &this->hdr_map;

        for(auto& lit_obj : this->lit_objects) {
            RayTraceMapper(lit_obj, this->ray_scene, 4, 4);
        }
    }
    CubeScene(CubeScene&) = delete;
    CubeScene(CubeScene&&) = delete;
    ~CubeScene() {
    }

    void Draw(const glm::mat4& view_mat, GLint model_loc, GLuint white_texture_id) {
        if(redraw || always_draw) {
            RayCamera cam;
            cam.SetFromMatrix(view_mat, this->fov, 1.0f, (float)this->width / (float)this->height);
            RayTraceSceneAccumulate(this->image, cam, this->ray_scene, 4, 10);
            tex = CreateTexture2D(image.colors, image.width, image.height);
            for(auto& lit_obj : this->lit_objects) {
                RayTraceMapper(lit_obj, this->ray_scene, 4, 4);
            }
            redraw = false;
        }



        glBindTexture(GL_TEXTURE_2D, white_texture_id);

        static GLenum cur_sample_mode = GL_LINEAR;
        uint32_t cur_obj_idx = 0;
        for(const auto& obj : this->ray_scene.objects) {
            if(draw_illuminance) {
                const auto& img = this->lit_objects.at(cur_obj_idx);
                if(cur_obj_idx < this->ray_textures.size()) {
                    this->ray_textures.at(cur_obj_idx) = CreateTexture2D(img.lightmap.colors, img.lightmap.width, img.lightmap.height);
                    glBindTexture(GL_TEXTURE_2D, this->ray_textures.at(cur_obj_idx).id);
                }
                else {
                    this->ray_textures.push_back(CreateTexture2D(img.lightmap.colors, img.lightmap.width, img.lightmap.height));
                    glBindTexture(GL_TEXTURE_2D, this->ray_textures.at(this->ray_textures.size() - 1).id);
                }
            }
            //glBindTexture(GL_TEXTURE_2D, this->debug_texture.id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, cur_sample_mode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, cur_sample_mode);
            glm::mat4 mat = glm::inverse(obj.inv_model_mat);
            glUniformMatrix4fv(model_loc, 1, GL_FALSE, (const GLfloat*)&mat);
            glBindVertexArray(this->cube_mesh.vao);
            glDrawElements(GL_TRIANGLES, this->cube_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
            cur_obj_idx += 1;
        }

        ImGui::Begin("CubeScene");
        ImGui::Image((ImTextureID)(uintptr_t)this->tex.id, {(float)this->width, (float)this->height});
        ImGui::Checkbox("Re-raytrace", &this->redraw);
        ImGui::Checkbox("always draw", &this->always_draw);
        ImGui::Checkbox("illuminance", &this->draw_illuminance);
        if(ImGui::Button("Clear")) {
            this->image.Clear();
        }
        if(ImGui::Button("Clear All")) {
            this->image.Clear();
            for(auto& obj : this->lit_objects) {
                obj.lightmap.Clear();
            }
        }
        if(ImGui::Button("Switch sampling mode")) {
            cur_sample_mode = cur_sample_mode == GL_LINEAR ? GL_NEAREST : GL_LINEAR;
        }
        if(ImGui::Button("PostProcess")) {
            for(auto& lit_obj : this->lit_objects) {
                this->post_processing.PostProcessLightMap(lit_obj.lightmap);
            }
        }
        ImGui::End();
    }

    
    RayImage post_processing;
    std::vector<LitObject> lit_objects;
    std::vector<Texture2D> ray_textures;
    Texture2D debug_texture;
    Texture2D hdr_map;
    RayScene ray_scene;
    BoundingVolumeHierarchy cube_bvh;
    RayImage image;
    uint32_t width = 100;
    uint32_t height = 100;
    Texture2D tex;
    Mesh cube_mesh;
    bool redraw = true;
    bool always_draw = false;
    bool draw_illuminance = false;
    float fov = glm::radians(45.0f);
};


static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

}


int main() {
    if(!glfwInit()) {
        std::cout << "failed to initialize glfw" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Graphics", nullptr, nullptr);
    if(!window) {
        std::cout << "failed to create window" << std::endl;
        return -1;
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if(!gladLoadGL()) {
        std::cout << "failed to load gl" << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(); 


    GLuint basic_program = LoadProgramFromFile("../assets/shaders/basic.vs", "../assets/shaders/basic.fs");
    GLint model_loc = glGetUniformLocation(basic_program, "model");
    GLint view_loc = glGetUniformLocation(basic_program, "view");
    GLint proj_loc = glGetUniformLocation(basic_program, "projection");

    //GLTFLoadData gltf_data = LoadGLTFLoadData("../assets/bee'maw.glb");
    //Mesh decimated_mesh = DecimateMesh(&gltf_data.meshes.at(0).mesh, 40000.0f);

    //BoundingBox model_bb = {{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
    //for(auto& m : gltf_data.meshes) {
    //    model_bb.max = glm::max(model_bb.max, m.mesh.bb.max);
    //    model_bb.min = glm::min(model_bb.min, m.mesh.bb.min);
    //}
    //glm::vec3 bb_hsz = (model_bb.max - model_bb.min) / 2.0f;
    //
    //glm::mat4 model = glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(0.001f)), -bb_hsz);

    glm::mat4 view = glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.01f, 100.0f);

    uint32_t col_white = 0xFFFFFFFF;
    Texture2D white_texture = CreateTexture2D(&col_white, 1, 1);

    CubeScene cube_scene;

    

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        static const char* rendering_modes[] = {
            "GL_FILL",
            "GL_LINE",
            "GL_POINT"
        };
        static GLenum rendering_modes_enum[] = {
            GL_FILL,
            GL_LINE,
            GL_POINT
        };
        static bool original = false;
        static int cur_rendering_mode = 0;
        //ImGui::Begin("rendering_mode");
        //ImGui::ListBox("mode", &cur_rendering_mode, rendering_modes, ARRSIZE(rendering_modes));
        //ImGui::Checkbox("Original", &original);
        //ImGui::End();

        GLenum rendering_mode = rendering_modes_enum[cur_rendering_mode];

        int win_width, win_height = 0;
        glfwGetWindowSize(window, &win_width, &win_height);
        glm::vec2 win_size = {static_cast<float>(win_width), static_cast<float>(win_height)};
        proj = glm::perspective(glm::radians(45.0f), win_size.x / win_size.y, 0.01f, 100.0f);

        static ImVec2 mouse_pos = ImGui::GetMousePos();
        const ImVec2 new_mouse_pos = ImGui::GetMousePos();
        const ImVec2 delta_mouse = {new_mouse_pos.x - mouse_pos.x, new_mouse_pos.y - mouse_pos.y};
        mouse_pos = new_mouse_pos;
        static glm::vec3 center = {};
        static float radius = 5.0f;
        static float phi = 0.0f;
        static float theta = M_PI / 4.0f;
        float camX = sin(theta) * sin(phi) * radius + center.x;
        float camY = cos(theta) * radius + center.y;
        float camZ = sin(theta) * cos(phi) * radius + center.z;
        if(ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const glm::mat4 inv_view_mat = glm::inverse(view);
            glm::vec3 right     = -glm::vec3(inv_view_mat[0][0], inv_view_mat[0][1], inv_view_mat[0][2]);
            glm::vec3 up        = glm::vec3(inv_view_mat[1][0], inv_view_mat[1][1], inv_view_mat[1][2]);
            glm::vec3 forward   = -glm::vec3(inv_view_mat[2][0], inv_view_mat[2][1], inv_view_mat[2][2]);

            constexpr float speed = 0.1f;
            float amplitude_forward = 0.0f;
            float amplitude_right = 0.0f;
            float amplitude_up = 0.0f;
            if(ImGui::IsKeyDown(ImGuiKey_W)) {
                amplitude_forward += speed;
            }
            if(ImGui::IsKeyDown(ImGuiKey_S)) {
                amplitude_forward -= speed;
            }
            if(ImGui::IsKeyDown(ImGuiKey_A)) {
                amplitude_right += speed;
            }
            if(ImGui::IsKeyDown(ImGuiKey_D)) {
                amplitude_right -= speed;
            }
            if(ImGui::IsKeyDown(ImGuiKey_Space)) {
                amplitude_up += speed;
            }
            if(ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
                amplitude_up -= speed;
            }
            const glm::vec3 delta_pos = forward * amplitude_forward + right * amplitude_right + up * amplitude_up;
            center += delta_pos;
            camX += delta_pos.x; camY += delta_pos.y; camZ += delta_pos.z;
        }
        if(ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            phi -= 0.02f * delta_mouse.x;
            theta -= 0.02f * delta_mouse.y;
        }
        if(ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            const glm::mat4 inv_view_mat = glm::inverse(view);
            glm::vec3 right     = -glm::vec3(inv_view_mat[0][0], inv_view_mat[0][1], inv_view_mat[0][2]);
            glm::vec3 up        = glm::vec3(inv_view_mat[1][0], inv_view_mat[1][1], inv_view_mat[1][2]);
            glm::vec3 forward   = -glm::vec3(inv_view_mat[2][0], inv_view_mat[2][1], inv_view_mat[2][2]);

            const glm::vec3 delta_pos = 0.1f * (delta_mouse.x * right + delta_mouse.y * up);
            center += delta_pos;
            camX += delta_pos.x; camY += delta_pos.y; camZ += delta_pos.z;
        }

    
        view = glm::lookAt(glm::vec3(camX, camY, camZ), center, glm::vec3(0.0, 1.0, 0.0));  


        glViewport(0, 0, win_width, win_height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        DrawHDR(cube_scene.hdr_map, view);
        glDepthMask(GL_TRUE);

        glEnable(GL_DEPTH_TEST);

        glUseProgram(basic_program);
        //glUniformMatrix4fv(model_loc, 1, GL_FALSE, (const GLfloat*)&model);
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, (const GLfloat*)&view);
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (const GLfloat*)&proj);

        glPolygonMode(GL_FRONT_AND_BACK, rendering_mode);
        cube_scene.Draw(view, model_loc, white_texture.id);


        //if(original) {
        //    auto& mesh = gltf_data.meshes.at(0);
        //    auto& mat = gltf_data.materials.at(mesh.material_idx);
        //    glBindTexture(GL_TEXTURE_2D, gltf_data.textures.at(mat.base_color.idx).id);

        //    glBindVertexArray(mesh.mesh.vao);
        //    glDrawElements(GL_TRIANGLES, mesh.mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
        //}
        //else {
        //    auto& mesh = decimated_mesh;
        //    auto& mat = gltf_data.materials.at(mesh.material_idx);
        //    glBindTexture(GL_TEXTURE_2D, gltf_data.textures.at(mat.base_color.idx).id);

        //    glBindVertexArray(mesh.vao);
        //    glDrawElements(GL_TRIANGLES, mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
        //}
        
        //for(auto& mesh : gltf_data.meshes) {
        //    auto& mat = gltf_data.materials.at(mesh.material_idx);
        //    glBindTexture(GL_TEXTURE_2D, gltf_data.textures.at(mat.base_color.idx).id);

        //    glBindVertexArray(mesh.mesh.vao);
        //    glDrawElements(GL_TRIANGLES, mesh.mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
        //}
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glBindVertexArray(0);


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        glfwSwapBuffers(window);
    }


    glDeleteProgram(basic_program);

    glfwTerminate();
    glfwDestroyWindow(window);
    

    return 0;
}
