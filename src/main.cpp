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
#include "utf8.h"
#include <vector>
#include <future>


static Texture2D hdr_map;
static Texture2D white_texture;
struct Scene {
    virtual void Draw(const glm::mat4& proj_mat, const glm::mat4& view_mat, const BasicShader& basic_shader, uint32_t width, uint32_t height) = 0;
};

struct RaytraceCubeScene : public Scene {
    RaytraceCubeScene() {
        std::vector<Vertex> small_verts;
        std::vector<uint32_t> small_inds;
        std::vector<Vertex> large_verts;
        std::vector<uint32_t> large_inds;
        const glm::vec4 cols[6] = {
            {1.0f, 0.0f, 1.0f, 1.0f},
            {0.0f, 1.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f, 1.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.0f, 1.0f, 1.0f, 1.0f},
        };
        GenerateCube(small_verts, small_inds, {1.0f, 1.0f, 1.0f}, cols);
        GenerateCube(large_verts, large_inds, {30.0f, 0.1f, 30.0f}, cols);

        ISize small_sz = GenerateUVs(small_verts, small_inds, 64);
        ISize large_sz = GenerateUVs(large_verts, large_inds, 128);
        std::cout << "small: " << small_sz.width << ", " << small_sz.height << std::endl;
        std::cout << "large: " << large_sz.width << ", " << large_sz.height << std::endl;
        this->post_processing = RayImage(glm::max(large_sz.width, small_sz.width), glm::max(large_sz.height, small_sz.height));

        this->small_cube_mesh = CreateMesh(small_verts.data(), small_inds.data(), small_verts.size(), small_inds.size());
        this->large_cube_mesh = CreateMesh(large_verts.data(), large_inds.data(), large_verts.size(), large_inds.size());
        small_cube_bvh.CreateFromMesh(&small_cube_mesh, 8);
        large_cube_bvh.CreateFromMesh(&large_cube_mesh, 8);
        RayObject obj = {};
        obj.bvh = &this->small_cube_bvh;
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


        mat = glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec3(0.0f, -0.05f, 0.0f));
        obj.bvh = &this->large_cube_bvh;
        obj.mat = mat;
        obj.inv_model_mat = glm::inverse(mat);
        obj.material.specular_col = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        obj.material.emission_col = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        obj.material.emission_strength = 0.0f;
        obj.material.smoothness = 0.0f;
        obj.material.specular_probability = 0.0f;
        this->ray_scene.objects.emplace_back(std::move(obj));
        this->lit_objects.push_back({RayImage(large_sz.width, large_sz.height), 2});

        this->ray_scene.hdr_map = &hdr_map;

        std::vector<std::future<void>> futures;
        for(size_t i = 0; i < this->lit_objects.size(); ++i) {
            futures.push_back(std::async(std::launch::async, [this, i]() {
                        RayTraceMapper(this->lit_objects.at(i), this->ray_scene, 2, 1);
            }));
            this->post_processed_imgs.push_back(RayImage(this->lit_objects.at(i).lightmap.width, this->lit_objects.at(i).lightmap.height));
        }
    }
    RaytraceCubeScene(RaytraceCubeScene&) = delete;
    RaytraceCubeScene(RaytraceCubeScene&&) = delete;
    ~RaytraceCubeScene() {
    }

    void DrawScene(const glm::mat4& view_mat, const BasicShader& basic_shader) {
        if(redraw || always_draw) {
            RayCamera cam;
            cam.SetFromMatrix(view_mat, this->fov, 1.0f, (float)this->width / (float)this->height);
            //RayTraceSceneAccumulate(this->image, cam, this->ray_scene, 4, 10);
            tex = CreateTexture2D(image.colors, image.width, image.height);
            {
                std::vector<std::future<void>> futures;
                for(size_t i = 0; i < this->lit_objects.size(); ++i) {
                    futures.push_back(std::async(std::launch::async, [this, i]() {
                        RayTraceMapper(this->lit_objects.at(i), this->ray_scene, 2, 1);

                        RayImage& light_copy = this->post_processed_imgs.at(i);
                        const RayImage& lightmap = this->lit_objects.at(i).lightmap;
                        light_copy.frame_scale = lightmap.frame_scale;
                        light_copy.num_rendered_frames = lightmap.num_rendered_frames;
                        for(size_t y = 0; y < lightmap.height; ++y) {
                            for(size_t x = 0; x < lightmap.width; ++x) {
                                light_copy.colors[y * lightmap.width + x] = lightmap.colors[y * lightmap.width + x];
                            }
                        }
                        RayImage post_processing_img(this->post_processing.width, this->post_processing.height);
                        for(size_t j = 0; j < 4; ++j) {
                            post_processing_img.PostProcessLightMap(light_copy);
                        }
                        for(size_t y = 0; y < lightmap.height; ++y) {
                            for(size_t x = 0; x < lightmap.width; ++x) {
                                light_copy.colors[y * lightmap.width + x].a = 1.0f;
                            }
                        }
                    }));
                }
            }
            redraw = false;
        }
        basic_shader.SetTexture(white_texture.id);

        static GLenum cur_sample_mode = GL_LINEAR;
        uint32_t cur_obj_idx = 0;
        for(const auto& obj : this->ray_scene.objects) {
            if(draw_illuminance) {
                const auto& img = this->post_processed_imgs.at(cur_obj_idx);
                if(cur_obj_idx < this->ray_textures.size()) {
                    this->ray_textures.at(cur_obj_idx) = CreateTexture2D(img.colors, img.width, img.height);
                    basic_shader.SetTexture(this->ray_textures.at(cur_obj_idx).id);
                }
                else {
                    this->ray_textures.push_back(CreateTexture2D(img.colors, img.width, img.height));
                    basic_shader.SetTexture(this->ray_textures.at(this->ray_textures.size() - 1).id);
                }
            }
            //basic_shader.SetTexture(this->debug_texture.id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, cur_sample_mode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, cur_sample_mode);
            glm::mat4 mat = glm::inverse(obj.inv_model_mat);
            basic_shader.SetModelMatrix(mat);
            if(obj.bvh == &this->small_cube_bvh) {
                glBindVertexArray(this->small_cube_mesh.vao);
                glDrawElements(GL_TRIANGLES, this->small_cube_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
            }
            else {
                glBindVertexArray(this->large_cube_mesh.vao);
                glDrawElements(GL_TRIANGLES, this->large_cube_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
            }
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
    virtual void Draw(const glm::mat4& proj_mat, const glm::mat4& view_mat, const BasicShader& basic_shader, uint32_t width, uint32_t height) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        DrawHDR(hdr_map, proj_mat, view_mat);
        glDepthMask(GL_TRUE);

        glEnable(GL_DEPTH_TEST);

        basic_shader.Bind();
        basic_shader.SetViewMatrix(view_mat);
        basic_shader.SetProjectionMatrix(proj_mat);

        this->DrawScene(view_mat, basic_shader);

        glBindVertexArray(0);

    }

    
    RayImage post_processing;
    std::vector<LitObject> lit_objects;
    std::vector<RayImage> post_processed_imgs;
    std::vector<Texture2D> ray_textures;
    Texture2D debug_texture;
    RayScene ray_scene;
    BoundingVolumeHierarchy small_cube_bvh;
    BoundingVolumeHierarchy large_cube_bvh;
    RayImage image;
    uint32_t width = 100;
    uint32_t height = 100;
    Texture2D tex;
    Mesh small_cube_mesh;
    Mesh large_cube_mesh;
    bool redraw = true;
    bool always_draw = false;
    bool draw_illuminance = false;
    float fov = glm::radians(45.0f);
};




struct IntersectionCubeScene : public Scene {
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

    void DrawScene() {
        glBindTexture(GL_TEXTURE_2D, white_texture.id);
        for(const glm::mat4& obj : objects) {
            shader.SetModelMatrix(obj);
            glBindVertexArray(this->cube_mesh.vao);
            glDrawElements(GL_TRIANGLES, this->cube_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
        }
    }
    virtual void Draw(const glm::mat4& proj_mat, const glm::mat4& view_mat, const BasicShader& basic_shader, uint32_t width, uint32_t height) override {
        glActiveTexture(GL_TEXTURE0);

        this->Update(width, height);
        this->DrawIntoDepth(basic_shader, view_mat, proj_mat);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        DrawHDR(hdr_map, proj_mat, view_mat);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        basic_shader.Bind();
        basic_shader.SetViewMatrix(view_mat);
        basic_shader.SetProjectionMatrix(proj_mat);

        this->DrawScene();

        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        this->DrawIntersectionMesh(view_mat, proj_mat);
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
    }
    void DrawIntoDepth(const BasicShader& shader, const glm::mat4& view, const glm::mat4& proj) {
        this->depth_tex.Begin({});
        shader.Bind();
        shader.SetViewMatrix(view);
        shader.SetProjectionMatrix(proj);
        this->DrawScene();
        this->depth_tex.End();
    }
    void DrawIntersectionMesh(const glm::mat4& view, const glm::mat4& proj) {
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
struct DecimationScene : public Scene {
    DecimationScene(uint32_t screen_width, uint32_t screen_height) : decimate_mesh(LoadGLTFLoadData(TranslateRelativePath("../../assets/couch.glb").c_str())) {
        this->mesh_idx = 2;
        this->fast_dyn_mesh.Initialize(&decimate_mesh.meshes.at(this->mesh_idx).mesh);
    }
    void DrawScene(const glm::mat4& proj_mat, const glm::mat4& view_mat) {
        const GLTFLoadData::GltfMesh& mesh = this->decimate_mesh.meshes.at(this->mesh_idx);
        trig_vis_shader.Bind();
        trig_vis_shader.SetModelMatrix(glm::mat4(1.0f));
        trig_vis_shader.SetViewMatrix(view_mat);
        trig_vis_shader.SetProjectionMatrix(proj_mat);
        glBindVertexArray(mesh.mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
    }
    virtual void Draw(const glm::mat4& proj_mat, const glm::mat4& view_mat, const BasicShader& basic_shader, uint32_t width, uint32_t height) override {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        DrawHDR(hdr_map, proj_mat, view_mat);
        glDepthMask(GL_TRUE);

        glEnable(GL_DEPTH_TEST);

        basic_shader.Bind();
        basic_shader.SetViewMatrix(view_mat);
        basic_shader.SetProjectionMatrix(proj_mat);

        this->DrawScene(proj_mat, view_mat);

        glBindVertexArray(0);
    }

    ~DecimationScene() {
    }
    TriangleVisibilityShader trig_vis_shader;
    FastDynamicMesh fast_dyn_mesh;
    uint32_t mesh_idx = 2;
    GLTFLoadData decimate_mesh;
};






static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {

}
static std::string test_string;
static void CharacterCallback(GLFWwindow* window, unsigned int codepoint) {
    utf8::append(codepoint, test_string);
}

enum CurrentActiveScene {
    CS_RaytraceCubeScene,
    CS_IntersectionCubeScene,
    CS_DecimationScene,
};
static CurrentActiveScene active_scene = CS_RaytraceCubeScene;

int main(int argc, char** argv) {
    SetExecutablePath(argv[0]);

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
    glfwSetCharCallback(window, CharacterCallback);
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
    white_texture = CreateTexture2D(&col_white, 1, 1);

    {
        int x,y,c;
        float* hdr_data = stbi_loadf(TranslateRelativePath("../../assets/garden.hdr").c_str(), &x, &y, &c, 4);
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

    Scene* scene = nullptr;
    if(active_scene == CurrentActiveScene::CS_IntersectionCubeScene) {
        scene = new IntersectionCubeScene(win_width, win_height);
    }
    else if(active_scene == CurrentActiveScene::CS_RaytraceCubeScene) {
        scene = new RaytraceCubeScene();
    }
    else if(active_scene == CurrentActiveScene::CS_DecimationScene) {
        scene = new DecimationScene(win_width, win_height);
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_CULL_FACE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    FontAtlasData font_data;
    Texture2D test_texture;
    int font_size = 16;
    //if(font_data.Load(test_texture, "../../assets/consola.ttf", font_size, false)) {
    if(font_data.Load(test_texture, TranslateRelativePath("../../assets/seguiemj.ttf"), font_size, false)) {
        std::cout << "successfully loaded: " << std::endl;
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

        scene->Draw(proj, view, basic_shader, win_width, win_height);

        if(false) { // nice looking text
            ImDrawList* draw_list = ImGui::GetForegroundDrawList();
            draw_list->AddRectFilled({0.0f, 0.0f}, {800.0f, 800.0f}, IM_COL32_WHITE);
            glm::vec2 start = {000.0f, 100.0f};
            auto begin = test_string.begin();
            auto end = test_string.end();
            float last_advance = 0.0f;
            for(uint32_t unicode = 0; begin != end;) {
                unicode = utf8::next(begin, end);
                if(unicode == ' ') {
                    start.x += font_size * 2 / 3;
                    continue;
                }
                FontAtlasData::Glyph* glyph = nullptr;
                for(auto& g : font_data.glyphs) {
                    if(g.unicode == unicode) {
                        glyph = &g;
                        break;
                    }
                }
                if(!glyph) {
                    std::cout << "not found: " << std::hex << unicode << std::endl;
                    continue;
                }
                if(0x300 <= unicode && unicode < 0x370) {
                    start.x -= last_advance;
                }
                glm::vec2 relative_start = glyph->start + start;
                glm::vec2 relative_end = relative_start + glyph->size;
                draw_list->AddImage((ImTextureID)test_texture.id, {relative_start.x, relative_start.y}, {relative_end.x, relative_end.y}, {glyph->start_uv.x, glyph->start_uv.y}, {glyph->end_uv.x, glyph->end_uv.y}, IM_COL32_BLACK);
                start.x += glyph->advance;
                last_advance = glyph->advance;
            }
        }


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        glfwSwapBuffers(window);
    }



    glfwTerminate();
    glfwDestroyWindow(window);
    
    return 0;
}
