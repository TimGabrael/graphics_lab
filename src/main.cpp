#include <iostream>
#include "util.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "mesh_decimation.h"
#include "raytracer.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "shaders.h"
#include "atlas.h"



struct RaytraceCubeScene {
    RaytraceCubeScene(const Texture2D* hdr_map) {
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
        small_sz.width /= 5;
        small_sz.height /= 5;
        int x,y,c;
        stbi_uc* data = stbi_load("example_uvmesh_tris00.tga", &x, &y, &c, 4);
        if(x > 0 && y > 0 && data) {
            this->debug_texture = CreateTexture2D((const uint32_t*)data, x, y);
            stbi_image_free(data);
        }
        else {
            std::cout << "failed to load debug_texture" << std::endl;
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

        this->ray_scene.hdr_map = hdr_map;

        for(auto& lit_obj : this->lit_objects) {
            RayTraceMapper(lit_obj, this->ray_scene, 4, 4);
        }
    }
    RaytraceCubeScene(RaytraceCubeScene&) = delete;
    RaytraceCubeScene(RaytraceCubeScene&&) = delete;
    ~RaytraceCubeScene() {
    }

    void Draw(const glm::mat4& view_mat, const BasicShader& basic_shader, GLuint white_texture_id) {
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
        basic_shader.SetTexture(white_texture_id);

        static GLenum cur_sample_mode = GL_LINEAR;
        uint32_t cur_obj_idx = 0;
        for(const auto& obj : this->ray_scene.objects) {
            if(draw_illuminance) {
                const auto& img = this->lit_objects.at(cur_obj_idx);
                if(cur_obj_idx < this->ray_textures.size()) {
                    this->ray_textures.at(cur_obj_idx) = CreateTexture2D(img.lightmap.colors, img.lightmap.width, img.lightmap.height);
                    basic_shader.SetTexture(this->ray_textures.at(cur_obj_idx).id);
                }
                else {
                    this->ray_textures.push_back(CreateTexture2D(img.lightmap.colors, img.lightmap.width, img.lightmap.height));
                    basic_shader.SetTexture(this->ray_textures.at(this->ray_textures.size() - 1).id);
                }
            }
            //basic_shader.SetTexture(this->debug_texture.id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, cur_sample_mode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, cur_sample_mode);
            glm::mat4 mat = glm::inverse(obj.inv_model_mat);
            basic_shader.SetModelMatrix(mat);
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


struct IntersectionCubeScene {
    IntersectionCubeScene(uint32_t screen_width, uint32_t screen_height) {
        std::vector<Vertex> verts;
        std::vector<uint32_t> inds;
        glm::vec4 cols[6] = {
            glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),
            glm::vec4(0.3f, 0.3f, 0.3f, 1.0f),
            glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
            glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
            glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        };
        GenerateCube(verts, inds, {1.0f, 1.0f, 1.0f}, cols);
        this->cube_mesh = CreateMesh(verts.data(), inds.data(), verts.size(), inds.size());
        verts.clear();
        inds.clear();
        glm::vec4 cols_intersect[6] = {
            glm::vec4(0.6f, 0.6f, 0.8f, 0.2f),
            glm::vec4(0.6f, 0.6f, 0.8f, 0.2f),
            glm::vec4(0.6f, 0.6f, 0.8f, 0.2f),
            glm::vec4(0.6f, 0.6f, 0.8f, 0.2f),
            glm::vec4(0.6f, 0.6f, 0.8f, 0.2f),
            glm::vec4(0.6f, 0.6f, 0.8f, 0.2f),
        };
        GenerateCube(verts, inds, {1.0f, 1.0f, 1.0f}, cols_intersect);
        this->intersection_mesh = CreateMesh(verts.data(), inds.data(), verts.size(), inds.size());
        this->depth_tex = CreateDepthTexture(screen_width, screen_height);

        this->objects.push_back(glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec3(0.0f, 0.5f, -3.0f)));
        this->objects.push_back(glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec3(0.0f, 0.5f, 10.0f)));
        this->objects.push_back(glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(30.0f, 0.1f, 30.0f)), glm::vec3(0.0f, -0.05f, 0.0f)));
        FastNoise fast_noise;
        fast_noise.GetSimplex(1.0f, 1.0f);
        float noise_texture_data[200*200];
        for(uint32_t y = 0; y < 200; ++y) {
            for(uint32_t x = 0; x < 200; ++x) {
                float cx = 2000.0f / 200.0f * x;
                float cy = 2000.0f / 200.0f * y;
                noise_texture_data[y * 200 + x] = fast_noise.GetSimplex(cx, cy);
            }
        }
        this->noise_texture = CreateTexture2D(noise_texture_data, 200, 200);

        const BoundingBox& bb = this->intersection_mesh.bb;
        this->data.max_extent = glm::length(bb.max - bb.min) / 2.0f;
        this->data.center = {0.0f, 0.0f, 0.0f};
    }
    void Update(uint32_t screen_width, uint32_t screen_height) {
        if(this->depth_tex.width != screen_width || this->depth_tex.height != screen_height) {
            this->depth_tex = CreateDepthTexture(screen_width, screen_height);
        }
    }

    void DrawScene(const BasicShader& shader, GLuint white_texture) const {
        glBindTexture(GL_TEXTURE_2D, white_texture);
        for(const glm::mat4& obj : objects) {
            shader.SetModelMatrix(obj);
            glBindVertexArray(this->cube_mesh.vao);
            glDrawElements(GL_TRIANGLES, this->cube_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
        }
    }
    void DrawIntoDepth(const BasicShader& shader, GLuint white_texture, const glm::mat4& view, const glm::mat4& proj) const {
        this->depth_tex.Begin({});
        shader.Bind();
        shader.SetViewMatrix(view);
        shader.SetProjectionMatrix(proj);
        this->DrawScene(shader, white_texture);
        this->depth_tex.End();
    }
    void DrawIntersectionMesh(const glm::mat4& view, const glm::mat4& proj, GLuint white_texture) {
        data.timer += 1.0f / 60.0f;
        static glm::vec3 translation = {0.2f, 0.0f, 0.0f};
        translation.z += 1.0f / 100.0f;
        if(translation.z > 15.0f) {
            translation.z = -10.0f;
        }
        this->shader.SetData(data);
        this->shader.Bind();
        this->shader.SetProjectionMatrix(proj);
        this->shader.SetViewMatrix(view);
        this->shader.SetModelMatrix(glm::translate(glm::identity<glm::mat4>(), translation));
        this->shader.SetColorTexture(this->noise_texture.id);
        this->shader.SetDepthTexture(this->depth_tex.depthbuffer_id);
        this->shader.SetScreenSize((float)this->depth_tex.width, (float)this->depth_tex.height);
        glBindVertexArray(this->intersection_mesh.vao);
        glDrawElements(GL_TRIANGLES, this->intersection_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
    }

    ~IntersectionCubeScene() {
    }
    IntersectionHightlightingShader::Data data;
    IntersectionHightlightingShader shader;
    std::vector<glm::mat4> objects;
    RenderTexture depth_tex;
    Texture2D noise_texture;
    Mesh cube_mesh;
    Mesh intersection_mesh;
};



static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

}

enum CurrentActiveScene {
    CS_RaytraceCubeScene,
    CS_IntersectionCubeScene,
};
static CurrentActiveScene active_scene = CS_IntersectionCubeScene;

int main() {
    if(!glfwInit()) {
        std::cout << "failed to initialize glfw" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    int win_width = 800;
    int win_height = 600;

    GLFWwindow* window = glfwCreateWindow(win_width, win_height, "Graphics", nullptr, nullptr);
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


    BasicShader basic_shader;

    glm::mat4 view = glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.01f, 100.0f);

    uint32_t col_white = 0xFFFFFFFF;
    Texture2D white_texture = CreateTexture2D(&col_white, 1, 1);


    Texture2D hdr_map;
    {
        int x,y,c;
        float* hdr_data = stbi_loadf("../../assets/garden.hdr", &x, &y, &c, 4);
        if(hdr_data) {
            // The sun is just WAAAY to bright in the hdr image,
            // and only a few rays hit it, but the ones that do get a absurdly high value assigned
            // and stay that way so it doesn't really converge nicely
            // (well it would if i took millions of samples, but that takes to long)
            for(size_t i = 0; i < x * y * c; ++i) {
                if(hdr_data[i] > 1.0f) {
                    hdr_data[i] = 1.0f;
                }
            }
            hdr_map = CreateTexture2D((const glm::vec4*)hdr_data, x, y);
            stbi_image_free(hdr_data);
        }
    }
    RaytraceCubeScene cube_scene(&hdr_map);
    IntersectionCubeScene intersect_cube_scene(win_width, win_height);

    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_CULL_FACE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    FontAtlasData font_data;
    Texture2D test_texture;
    if(font_data.Load(test_texture, "../../assets/consola.ttf", 12, false)) {
        std::cout << "successfully loaded" << std::endl;
        std::cout << test_texture.width << ", " << test_texture.height << std::endl;
    }

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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
        glm::vec3 cam_pos = glm::vec3(sin(theta) * sin(phi), cos(theta), sin(theta) * cos(phi)) * radius + center;
        if(ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const glm::mat4 inv_view_mat = glm::inverse(view);
            glm::vec3 right = -glm::vec3(inv_view_mat[0][0], inv_view_mat[0][1], inv_view_mat[0][2]);
            right = glm::normalize(glm::vec3(right.x, 0.0f, right.z));
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

            phi -= 0.01f * delta_mouse.x;
            theta -= 0.01f * delta_mouse.y;
            glm::vec3 forward = -glm::vec3(sin(theta) * sin(phi), cos(theta), sin(theta) * cos(phi));
            center = forward * radius + cam_pos;

            forward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));

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
            const glm::vec3 delta_pos = forward * amplitude_forward + right * amplitude_right + glm::vec3(0.0f, 1.0f, 0.0f) * amplitude_up;
            center += delta_pos;
            cam_pos += delta_pos;

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
            cam_pos += delta_pos;
        }

        view = glm::lookAt(cam_pos, center, glm::vec3(0.0, 1.0, 0.0));  

        if(active_scene == CS_IntersectionCubeScene) {
            glActiveTexture(GL_TEXTURE0);

            intersect_cube_scene.Update(win_width, win_height);
            intersect_cube_scene.DrawIntoDepth(basic_shader, white_texture.id, view, proj);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, win_width, win_height);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClearDepthf(1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            glDepthMask(GL_FALSE);
            DrawHDR(hdr_map, view);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);

            basic_shader.Bind();
            basic_shader.SetViewMatrix(view);
            basic_shader.SetProjectionMatrix(proj);

            intersect_cube_scene.DrawScene(basic_shader, white_texture.id);

            glDepthMask(GL_FALSE);
            glDisable(GL_CULL_FACE);
            intersect_cube_scene.DrawIntersectionMesh(view, proj, white_texture.id);
            glEnable(GL_CULL_FACE);
            glDepthMask(GL_TRUE);
        }
        else if(active_scene == CS_RaytraceCubeScene) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, win_width, win_height);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClearDepthf(1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            glDepthMask(GL_FALSE);
            DrawHDR(hdr_map, view);
            glDepthMask(GL_TRUE);

            glEnable(GL_DEPTH_TEST);

            basic_shader.Bind();
            basic_shader.SetViewMatrix(view);
            basic_shader.SetProjectionMatrix(proj);

            cube_scene.Draw(view, basic_shader, white_texture.id);

            glBindVertexArray(0);
        }

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        draw_list->AddRectFilled({0.0f, 0.0f}, {800.0f, 800.0f}, IM_COL32_WHITE);
        static std::string test_string = "Suppenhuhn ist der Meister des Waldes!";
        glm::vec2 start = {100.0f, 100.0f};
        for(char c : test_string) {
            if(c == ' ') {
                start.x += 12;
                continue;
            }
            FontAtlasData::Glyph* glyph = nullptr;
            for(auto& g : font_data.glyphs) {
                if(g.unicode == c) {
                    glyph = &g;
                }
            }
            if(!glyph) {
                continue;
            }
            glm::vec2 relative_start = glyph->start + start;
            glm::vec2 relative_end = relative_start + glyph->size;
            
            draw_list->AddImage((ImTextureID)test_texture.id, {relative_start.x, relative_start.y}, {relative_end.x, relative_end.y}, {glyph->start_uv.x, glyph->start_uv.y}, {glyph->end_uv.x, glyph->end_uv.y}, IM_COL32_BLACK);
            start.x += glyph->advance;
        }

        //ImGui::Begin("test_window");
        //ImGui::Image((ImTextureID)test_texture.id, {(float)test_texture.width, (float)test_texture.height});
        //ImGui::End();


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        glfwSwapBuffers(window);
    }



    glfwTerminate();
    glfwDestroyWindow(window);
    
    return 0;
}
