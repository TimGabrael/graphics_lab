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

static constexpr float NEAR_PLANE = 0.01f;
static constexpr float FAR_PLANE = 100.0f;
static constexpr float FOV = glm::radians(45.0f);

static Texture2D hdr_map;
static Texture2D white_texture;
struct Scene {
    virtual void Draw(const glm::mat4& proj_mat, const glm::mat4& view_mat, const BasicShader& basic_shader, uint32_t width, uint32_t height) = 0;
    virtual void Update(float dt) = 0;
    virtual void KeyCallback(int key, int scancode, int action, int mods) = 0;
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
        //this->redraw = false;
        //this->always_draw = false;
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

        this->DrawScene(view_mat, basic_shader);

        glBindVertexArray(0);
    }
    virtual void Update(float dt) override {
    }
    virtual void KeyCallback(int key, int scancode, int action, int mods) override {
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
    void UpdateTexture(uint32_t screen_width, uint32_t screen_height) {
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

        this->UpdateTexture(width, height);
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
    virtual void Update(float dt) override {
    }
    virtual void KeyCallback(int key, int scancode, int action, int mods) override {
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
    DecimationScene(uint32_t screen_width, uint32_t screen_height) : decimate_mesh(LoadGLTFLoadData(TranslateRelativePath("../../assets/Duck.glb").c_str())) {
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

        this->mesh_idx = 0;
        this->fast_dyn_mesh.Initialize(&decimate_mesh.meshes.at(this->mesh_idx).mesh);
    }
    void DrawScene(const glm::mat4& proj_mat, const glm::mat4& view_mat) {
        //const GLTFLoadData::GltfMesh& mesh = this->decimate_mesh.meshes.at(this->mesh_idx);
        //trig_vis_shader.Bind();
        //trig_vis_shader.SetModelMatrix(glm::mat4(1.0f));
        //trig_vis_shader.SetViewMatrix(view_mat);
        //trig_vis_shader.SetProjectionMatrix(proj_mat);
        //glBindVertexArray(mesh.mesh.vao);
        //glDrawElements(GL_TRIANGLES, mesh.mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);

        static Mesh decimated_mesh = this->fast_dyn_mesh.Decimate(0.0f);
        const Mesh& render_mesh = decimated_mesh;
        std::cout << "decimated_triangle_count: " << render_mesh.triangle_count << std::endl;
        std::cout << "original_triangle_count: " << this->decimate_mesh.meshes.at(0).mesh.triangle_count << std::endl;

        trig_vis_shader.Bind();
        trig_vis_shader.SetModelMatrix(glm::mat4(1.0f));
        trig_vis_shader.SetViewMatrix(view_mat);
        trig_vis_shader.SetProjectionMatrix(proj_mat);
        glBindVertexArray(render_mesh.vao);
        glDrawElements(GL_TRIANGLES, render_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);

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
    virtual void Update(float dt) override {
    }
    virtual void KeyCallback(int key, int scancode, int action, int mods) override {
    }

    ~DecimationScene() {
    }
    TriangleVisibilityShader trig_vis_shader;
    Mesh cube_mesh; // simple mesh for testing purposes
    FastDynamicMesh fast_dyn_mesh;
    uint32_t mesh_idx = 2;
    GLTFLoadData decimate_mesh;
};

struct VolumetricFog : public Scene {
    VolumetricFog(uint32_t width, uint32_t height) {
        fog_texture = CreateDepthTexture(width, height);
        
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

        fog_data.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        fog_data.iTime = 0.0f;
        fog_data.center_and_radius = glm::vec4(0.0f, 0.0f, 0.0f, 2.0f);
        fog_data.iFrame = 0;

        this->objects.push_back(glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec3(0.0f, 0.5f, -3.0f)));
        this->objects.push_back(glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f, 1.0f, 1.0f)), glm::vec3(0.0f, 0.5f, 10.0f)));
        this->objects.push_back(glm::translate(glm::scale(glm::identity<glm::mat4>(), glm::vec3(30.0f, 0.1f, 30.0f)), glm::vec3(0.0f, -0.05f, 0.0f)));
    }
    ~VolumetricFog() {
    }
    virtual void Draw(const glm::mat4& proj_mat, const glm::mat4& view_mat, const BasicShader& basic_shader, uint32_t width, uint32_t height) override {
        fog_data.iTime += 1.0f / 60.0f; // dt
        fog_data.iFrame += 1;
        glBindFramebuffer(GL_FRAMEBUFFER, this->fog_texture.framebuffer_id);
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, white_texture.id);
        basic_shader.Bind();
        basic_shader.SetViewMatrix(view_mat);
        basic_shader.SetProjectionMatrix(proj_mat);
        for(const glm::mat4& obj : this->objects) {
            basic_shader.SetModelMatrix(obj);
            glBindVertexArray(this->cube_mesh.vao);
            glDrawElements(GL_TRIANGLES, this->cube_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
        }


        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        DrawHDR(hdr_map, proj_mat, view_mat);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        glBindTexture(GL_TEXTURE_2D, white_texture.id);
        basic_shader.Bind();
        basic_shader.SetViewMatrix(view_mat);
        basic_shader.SetProjectionMatrix(proj_mat);
        for(const glm::mat4& obj : this->objects) {
            basic_shader.SetModelMatrix(obj);
            glBindVertexArray(this->cube_mesh.vao);
            glDrawElements(GL_TRIANGLES, this->cube_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
        }

        glBindTexture(GL_TEXTURE_2D, fog_texture.depthbuffer_id);
        fog_shader.Bind();
        fog_shader.SetData(fog_data);
        fog_shader.SetScreenSize({width, height});
        fog_shader.SetViewMatrix(view_mat);
        fog_shader.SetProjMat(proj_mat);
        fog_shader.Draw();

    }
    virtual void Update(float dt) override {
    }
    virtual void KeyCallback(int key, int scancode, int action, int mods) override {
        if(key == GLFW_KEY_O && action == GLFW_PRESS) {
            fog_shader.~VolumetricFogShader();
            new(&fog_shader)VolumetricFogShader();
        }
    }

    VolumetricFogShader fog_shader;
    VolumetricFogShader::FogData fog_data;
    RenderTexture fog_texture;
    Mesh cube_mesh;
    std::vector<glm::mat4> objects;
};
struct CascadedShadowMap : public Scene {
    CascadedShadowMap() {
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
        this->objects.push_back(glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(200.0f, 0.1f, 200.0f)), glm::vec3(0.0f, -0.1f, 0.0f)));
        this->objects.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.51f, 0.0f)));

        this->shadow_map = CreateDepthTexture(4096, 4096);
        this->light_dir = glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f));

        // setup the sampling for the shadow map
        glBindTexture(GL_TEXTURE_2D, this->shadow_map.depthbuffer_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    }
    ~CascadedShadowMap() {
        DestroyRenderTexture(&this->shadow_map);
        DestroyMesh(&this->cube_mesh);
    }

    glm::mat4 CalculateLightSpaceMatrix(const glm::vec3& light_dir, const glm::mat4& view_mat, const glm::mat4& proj_mat, float near_plane, float far_plane, glm::mat4& out_view, glm::mat4& out_proj) {
        glm::mat4 inv_vp = glm::inverse(proj_mat * view_mat);
        glm::vec4 frustum_corners[8] = {
            { -1.0f, -1.0f, 0.0f, 1.0f }, {  1.0f, -1.0f, 0.0f, 1.0f },
            { -1.0f,  1.0f, 0.0f, 1.0f }, {  1.0f,  1.0f, 0.0f, 1.0f },
            { -1.0f, -1.0f, 1.0f, 1.0f }, {  1.0f, -1.0f, 1.0f, 1.0f },
            { -1.0f,  1.0f, 1.0f, 1.0f }, {  1.0f,  1.0f, 1.0f, 1.0f }
        };

        const glm::mat4 light_view = glm::lookAt(light_dir * 1.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec3 min_bounds = glm::vec3( FLT_MAX);
        glm::vec3 max_bounds = glm::vec3(-FLT_MAX);
        for(size_t i = 0; i < ARRSIZE(frustum_corners); ++i) {
            glm::vec4 world_pos = inv_vp * frustum_corners[i];
            world_pos /= world_pos.w;
            glm::vec3 corner = glm::vec3(world_pos);
            float z = glm::mix(near_plane, far_plane, (corner.z - near_plane) / (far_plane - near_plane));
            corner.z = z;

            const glm::vec4 light_space_corner = light_view * glm::vec4(corner, 1.0f);
            const glm::vec3 light_frustum_corner = glm::vec3(light_space_corner);
            min_bounds = glm::min(min_bounds, light_frustum_corner);
            max_bounds = glm::max(max_bounds, light_frustum_corner);
        }

        const glm::mat4 light_proj = glm::ortho(min_bounds.x, max_bounds.x, max_bounds.y, min_bounds.y, min_bounds.z, max_bounds.z);

        out_view = light_view;
        out_proj = light_proj;

        return light_proj * light_view;
    }
    void DrawToShadowMap() const {
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_map.framebuffer_id);
        glViewport(0, 0, shadow_map.width, shadow_map.height);
        glClearDepthf(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        this->depth_only_shader.Bind();
        const glm::vec2 shadow_tex_size = glm::vec2(shadow_map.width, shadow_map.height);
        for(size_t i = 0; i < ARRSIZE(light_projections); ++i) {
            const glm::vec2 start = glm::vec2(light_bounds[i].x, light_bounds[i].y) * shadow_tex_size;
            const glm::vec2 end = glm::vec2(light_bounds[i].z, light_bounds[i].w) * shadow_tex_size;
            const glm::vec2 size = end - start;
            glViewport(static_cast<GLint>(start.x), static_cast<GLint>(start.y), static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));
            this->depth_only_shader.SetViewProjMatrix(light_projections[i]);
            for(const glm::mat4& obj : this->objects) {
                this->depth_only_shader.SetModelMatrix(obj);
                glBindVertexArray(this->cube_mesh.vao);
                glDrawElements(GL_TRIANGLES, this->cube_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
            }
        }
    }

    void DrawScene(const glm::mat4& proj, const glm::mat4& view, uint32_t win_width, uint32_t win_height) const {
        glCullFace(GL_FRONT);
        
        
        glm::mat4 render_view = view;
        glm::mat4 render_proj = proj;
        if(this->light_debug_view < ARRSIZE(light_view)) {
            render_view = light_view[this->light_debug_view];
            render_proj = light_proj[this->light_debug_view];
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, win_width, win_height);
        glClearDepthf(1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glDepthMask(GL_FALSE);
        DrawHDR(hdr_map, proj, render_view);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        this->shader.Bind();
        this->shader.SetViewMatrix(render_view);
        this->shader.SetProjMatrix(render_proj);
        this->shader.SetLightProjAndBounds(this->light_projections, this->light_bounds);
        this->shader.SetColorTexture(white_texture.id);
        this->shader.SetShadowMap(this->shadow_map.depthbuffer_id);
        this->shader.SetLightDirection(this->light_dir);
        this->shader.SetDebugView(this->debug_view);
        for(const glm::mat4& obj : this->objects) {
            this->shader.SetModelMatrix(obj);
            glBindVertexArray(this->cube_mesh.vao);
            glDrawElements(GL_TRIANGLES, this->cube_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
        }
    }
    void CalculateLightProjections(const glm::mat4& proj, const glm::mat4& view) {
        const glm::mat4 inv_cam = glm::inverse(proj * view);

        float cascade_splits[NUM_CASCADES] = {};
        const float near_clip = NEAR_PLANE;
        const float far_clip = FAR_PLANE;
        const float clip_range = far_clip - near_clip;
        const float min_z = near_clip;
        const float max_z = near_clip + clip_range;
        const float range = max_z - min_z;
        const float ratio = max_z / min_z;
        for(uint32_t i = 0; i < NUM_CASCADES; ++i) {
            const float p = (i + 1) / static_cast<float>(NUM_CASCADES);
            const float log = min_z * std::pow(ratio, p);
            const float uniform = min_z + range * p;
            const float d = split_lambda * (log - uniform) + uniform;
            cascade_splits[i] = (d - near_clip) / clip_range;
        }
        float last_split_dist = 0.0f;
        for(uint32_t i = 0; i < NUM_CASCADES; ++i) {
            const float split_dist = cascade_splits[i];
            glm::vec3 frustum_corners[8] = {
                glm::vec3(-1.0f,  1.0f, 0.0f),
                glm::vec3( 1.0f,  1.0f, 0.0f),
                glm::vec3( 1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f,  1.0f,  1.0f),
                glm::vec3( 1.0f,  1.0f,  1.0f),
                glm::vec3( 1.0f, -1.0f,  1.0f),
                glm::vec3(-1.0f, -1.0f,  1.0f),
            };

            for(size_t j = 0; j < ARRSIZE(frustum_corners); ++j) {
                const glm::vec4 inv_corner = inv_cam * glm::vec4(frustum_corners[j], 1.0f);
                frustum_corners[j] = inv_corner / inv_corner.w;
            }
            for(uint32_t j = 0; j < 4; ++j) {
                const glm::vec3 dist = frustum_corners[j + 4] - frustum_corners[j];
                frustum_corners[j + 4] = frustum_corners[j] + (dist * split_dist);
                frustum_corners[j] = frustum_corners[j] + (dist * last_split_dist);
            }
            glm::vec3 frustum_center = glm::vec3(0.0f);
            for(uint32_t j = 0; j < 8; ++j) {
                frustum_center += frustum_corners[j];
            }
            frustum_center /= 8.0f;
            float radius = 0.0f;
            for(uint32_t j = 0; j < 8; ++j) {
                const float distance = glm::length(frustum_corners[j] - frustum_center);
                radius = glm::max(radius, distance);
            }
            const glm::vec3 max_extents = glm::vec3(radius);
            const glm::vec3 min_extents = -max_extents;

            this->light_view[i] = glm::lookAt(frustum_center + light_dir * min_extents.z, frustum_center, glm::vec3(0.0f, 1.0f, 0.0f));
            this->light_proj[i] = glm::ortho(min_extents.x, max_extents.x, min_extents.y, max_extents.y, 0.0f, max_extents.z - min_extents.z);

            this->light_projections[i] = this->light_proj[i] * this->light_view[i];

            uint32_t x = i % 2;
            uint32_t y = i / 2;
            this->light_bounds[i] = glm::vec4(x * 0.5f, y * 0.5f, (x+1) * 0.5f, (y+1) * 0.5f);

            last_split_dist = cascade_splits[i];
        }
    }
    virtual void Draw(const glm::mat4& proj_mat, const glm::mat4& view_mat, const BasicShader& basic_shader, uint32_t width, uint32_t height) override {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);
        
        CalculateLightProjections(proj_mat, view_mat);
        DrawToShadowMap();
        DrawScene(proj_mat, view_mat, width, height);
    }
    virtual void Update(float dt) override {
        cur_light_angle += dt * 0.1f;
        this->light_dir = glm::normalize(glm::vec3(cosf(cur_light_angle), -1.0f, sinf(cur_light_angle)));
    }
    virtual void KeyCallback(int key, int scancode, int action, int mods) override {
        if(key == GLFW_KEY_O && action == GLFW_PRESS) {
            light_debug_view += 1;
            if(light_debug_view >= ARRSIZE(light_view)) {
                light_debug_view = UINT32_MAX;
            }
        }
        if(key == GLFW_KEY_K && action == GLFW_PRESS) {
            this->debug_view = !this->debug_view;
        }
    }


    static constexpr uint32_t NUM_CASCADES = 4;
    glm::vec3 light_dir;
    float cur_light_angle = 0.0f;
    glm::mat4 light_view[NUM_CASCADES];
    glm::mat4 light_proj[NUM_CASCADES];
    uint32_t light_debug_view = UINT32_MAX;
    glm::mat4 light_projections[NUM_CASCADES];
    glm::vec4 light_bounds[NUM_CASCADES];
    bool debug_view = false;
    float split_lambda = 0.95f;
    CascadedShadowShader shader;
    DepthOnlyShader depth_only_shader;
    RenderTexture shadow_map;
    Mesh cube_mesh;
    std::vector<glm::mat4> objects;
};

struct FluidSimScene : public Scene {
    struct Density {
        float far;
        float near;
    };
    FluidSimScene(uint32_t count_x, uint32_t count_y, uint32_t count_z) {
        this->bb.min = glm::vec3(-1.0f);
        this->bb.max = glm::vec3( 1.0f);
        const glm::vec3 count = glm::vec3(static_cast<float>(count_x), static_cast<float>(count_y), static_cast<float>(count_z));
        const glm::vec3 start = this->bb.min;
        const glm::vec3 delta = (this->bb.max - this->bb.min) / count;
        this->radius = delta / 2.0f;

        std::vector<Vertex> verts;
        std::vector<uint32_t> inds;
        //GenerateSphere(verts, inds, {}, this->radius, {1.0f, 1.0f, 1.0f, 1.0f}, 8);
        GenerateIcoSphere(verts, inds, {}, this->radius, {1.0f, 1.0f, 1.0f, 1.0f}, true);

        this->base_mesh = CreateMesh(verts.data(), inds.data(), verts.size(), inds.size());
        glGenBuffers(1, &this->instance_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, this->instance_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * this->particle_positions.size(), this->particle_positions.data(), GL_STATIC_DRAW);

        glBindVertexArray(this->base_mesh.vao);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(BasicShaderInstanced::InstanceData), 0);
        glEnableVertexAttribArray(4);
        glVertexAttribDivisor(4, 1);

        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(BasicShaderInstanced::InstanceData), (void*)offsetof(BasicShaderInstanced::InstanceData, col));
        glEnableVertexAttribArray(5);
        glVertexAttribDivisor(5, 1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);


        for(uint32_t z = 0; z < count_z; ++z) {
            for(uint32_t y = 0; y < count_y; ++y) {
                for(uint32_t x = 0; x < count_x; ++x) {
                    const glm::vec3 ind = glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                    const glm::vec3 cur = start + delta * ind;
                    if(glm::dot(cur, cur) < 0.5f) {
                        this->particle_positions.push_back(cur);
                        this->particle_velocities.push_back({});
                    }
                }
            }
        }
    }
    ~FluidSimScene() {
        glDeleteBuffers(1, &this->instance_buffer);
    }


    static float SmoothingKernelPoly6(float dst, float radius) {
        if (dst < radius) {
            const float scale = 315.0f / (64.0f * M_PI * glm::abs(radius * radius * radius * radius * radius * radius * radius * radius * radius));
            const float v = radius * radius - dst * dst;
            return v * v * v * scale;
        }
        return 0.0f;
    }
    static float DensityKernel(float dst, float radius) {
        if(dst < radius) {
            const float scale = 15.0f / (2.0f * M_PI * radius * radius * radius * radius * radius);
            const float v = radius - dst;
            return v * v * scale;
        }
        return 0.0f;
    }
    static float NearDensityKernel(float dst, float radius) {
        if(dst < radius) {
            const float scale = 15.0f / (M_PI * radius * radius * radius * radius * radius * radius);
            const float v = radius - dst;
            return v * v * v * scale;
        }
        return 0.0f;
    }
    static float DensityDerivative(float dst, float radius) {
        if (dst <= radius) {
            const float scale = 15.0f / (M_PI * radius * radius * radius * radius * radius);
            const float v = radius - dst;
            return -v * scale;
        }
        return 0.0f;
    }
    static float NearDensityDerivative(float dst, float radius) {
        if (dst <= radius) {
            const float scale = 45.0f / (M_PI * radius * radius * radius * radius * radius * radius);
            const float v = radius - dst;
            return -v * v * scale;
        }
        return 0;
    }

    float PressureFromDensity(float density) {
        return (density - this->target_density) * this->pressure_multiplier;
    }
    float NearPressureFromDensity(float near_density) {
        return near_density * this->near_pressure_multiplier;
    }
    void ExternalForces(float dt) {
        for(size_t i = 0; i < this->particle_velocities.size(); ++i) {
            this->particle_velocities.at(i) += this->gravity * dt;
            this->predicted_positions.at(i) = this->particle_positions.at(i) + this->particle_velocities.at(i) * 1.0f / 120.0f;
        }
    }
    void CalculateDensities(float dt) {
        const float square_radius = this->smoothing_radius * this->smoothing_radius;
        for(size_t i = 0; i < this->particle_positions.size(); ++i) {
            const glm::vec3& pos = this->predicted_positions.at(i);
            Density density = {0.0f, 0.0f};
            for(size_t j = 0; j < this->particle_positions.size(); ++j) {
                const glm::vec3& predicted_pos = this->predicted_positions.at(j);
                const glm::vec3 offset_to_neighbour = predicted_pos - pos;
                const float square_dist = glm::dot(offset_to_neighbour, offset_to_neighbour);
                if(square_dist > square_radius) {
                    continue;
                }
                const float dst = glm::sqrt(square_dist);
                density.far += DensityKernel(dst, this->smoothing_radius);
                density.near += NearDensityKernel(dst, this->smoothing_radius);
            }
            this->particle_densities.at(i) = density;
        }
    }
    void CalculatePressureForce(float dt) {
        const float square_radius = this->smoothing_radius * this->smoothing_radius;
        for(size_t i = 0; i < this->particle_positions.size(); ++i) {
            const Density& density = this->particle_densities.at(i);
            const float pressure = PressureFromDensity(density.far);
            const float near_pressure = NearPressureFromDensity(density.near);
            const glm::vec3& pos = this->predicted_positions.at(i);
            
            glm::vec3 pressure_force = {};
            for(size_t j = 0; j < this->particle_positions.size(); ++j) {
                if(i == j) {
                    continue;
                }
                const glm::vec3& neighbour_pos = this->predicted_positions.at(j);
                const glm::vec3 offset_to_neighbour = neighbour_pos - pos;
                const float square_dist = glm::dot(offset_to_neighbour, offset_to_neighbour);
                if(square_dist > square_radius) {
                    continue;
                }
                const Density& neighbour_density = this->particle_densities.at(j);
                const float neighbour_pressure = PressureFromDensity(neighbour_density.far);
                const float near_neighbour_pressure = NearPressureFromDensity(neighbour_density.near);
                const float shared_pressure = (pressure + neighbour_pressure) / 2.0f;
                const float near_shared_pressure = (near_pressure + near_neighbour_pressure) / 2.0f;
                const float dist = glm::sqrt(square_dist);
                const glm::vec3 dir = dist > 0.0f ? offset_to_neighbour / dist : glm::vec3(0.0f, 1.0f, 0.0f);
                pressure_force += dir * DensityDerivative(dist, this->smoothing_radius) * shared_pressure / neighbour_density.far;
                pressure_force += dir * NearDensityDerivative(dist, this->smoothing_radius) * near_shared_pressure / neighbour_density.near;
            }
            const glm::vec3 acceleration = pressure_force / density.far;
            this->particle_velocities.at(i) += acceleration * dt;
        }
    }
    void CalculateViscosity(float dt) {
        const float square_radius = this->smoothing_radius * this->smoothing_radius;
        for(size_t i = 0; i < this->particle_positions.size(); ++i) {
            const glm::vec3& pos = this->predicted_positions.at(i);
            const glm::vec3& velocity = this->particle_velocities.at(i);
            glm::vec3 viscosity_force = {};
            
            for(size_t j = 0; j < this->particle_positions.size(); ++j) {
                if(i == j) {
                    continue;
                }
                const glm::vec3& neighbour_pos = this->predicted_positions.at(j);
                const glm::vec3 offset_to_neighbour = neighbour_pos - pos;
                const float square_dist = glm::dot(offset_to_neighbour, offset_to_neighbour);
                if(square_dist > square_radius) {
                    continue;
                }
                const float dist = glm::sqrt(square_dist);
                const glm::vec3& neighbour_velocity = this->particle_velocities.at(j);
                viscosity_force += (neighbour_velocity - velocity) * SmoothingKernelPoly6(dist, this->smoothing_radius);
            }
            this->particle_velocities.at(i) += viscosity_force * this->viscosity_strength * dt;
        }
    }


    void FixedUpdate(float dt) {
        if(this->particle_velocities.size() != this->particle_positions.size()) {
            this->particle_velocities.resize(this->particle_positions.size(), glm::vec3(0.0f));
        }
        if(this->particle_densities.size() != this->particle_positions.size()) {
            this->particle_densities.resize(this->particle_positions.size(), {0.0f, 0.0f});
        }
        if(this->predicted_positions.size() != this->particle_positions.size()) {
            this->predicted_positions.resize(this->particle_positions.size(), glm::vec3(0.0f));
        }

        this->ExternalForces(dt);
        this->CalculateDensities(dt);
        this->CalculatePressureForce(dt);
        this->CalculateViscosity(dt);

        bool is_nan_once = false;
        for(size_t i = 0; i < this->particle_positions.size(); ++i) {
            glm::vec3& vel = this->particle_velocities.at(i);
            const glm::vec3 delta_pos = vel * dt;

            if(glm::isnan(vel.x) || glm::isnan(vel.y) || glm::isnan(vel.z)) {
                is_nan_once = true;
            }
            glm::vec3 new_pos = this->particle_positions.at(i) + delta_pos;
            if(new_pos.x < this->bb.min.x) {
                vel.x = -vel.x * this->collision_damping;
                new_pos.x = 2.0f * this->bb.min.x - new_pos.x;
            }
            if(new_pos.y < this->bb.min.y) {
                vel.y = -vel.y * this->collision_damping;
                new_pos.y = 2.0f * this->bb.min.y - new_pos.y;
            }
            if(new_pos.z < this->bb.min.z) {
                vel.z = -vel.z * this->collision_damping;
                new_pos.z = 2.0f * this->bb.min.z - new_pos.z;
            }
            if(new_pos.x > this->bb.max.x) {
                vel.x = -vel.x * this->collision_damping;
                new_pos.x = 2.0f * this->bb.max.x - new_pos.x;
            }
            if(new_pos.y > this->bb.max.y) {
                vel.y = -vel.y * this->collision_damping;
                new_pos.y = 2.0f * this->bb.max.y - new_pos.y;
            }
            if(new_pos.z > this->bb.max.z) {
                vel.z = -vel.z * this->collision_damping;
                new_pos.z = 2.0f * this->bb.max.z - new_pos.z;
            }
            this->particle_positions.at(i) = new_pos;
        }
        if(is_nan_once) {
            std::cout << "is nan" << std::endl;
        }
    }
    virtual void Update(float dt) override {
        const float fixed_timestep = 1.0f / 60.0f;
        static float timer = 0.0f;
        timer += dt;
        if(timer > fixed_timestep * 5.0f) {
            timer = 0.0f;
        }
        while(timer > fixed_timestep) {
            this->FixedUpdate(fixed_timestep);
            timer -= fixed_timestep;
        }
    }
    virtual void Draw(const glm::mat4& proj_mat, const glm::mat4& view_mat, const BasicShader& basic_shader, uint32_t width, uint32_t height) override {

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        DrawHDR(hdr_map, proj_mat, view_mat);

        std::vector<BasicShaderInstanced::InstanceData> instance_data(particle_positions.size());
        for(size_t i = 0; i < particle_positions.size(); ++i) {
            instance_data.at(i).pos = particle_positions.at(i);
            const float len = glm::min(glm::length(this->particle_velocities.at(i)) / 10.0f, 1.0f);

            const glm::vec4 blue_hsla = RgbaToHsla(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
            const glm::vec4 red_hsla = RgbaToHsla(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
            instance_data.at(i).col = HslaToRgba({glm::mix(blue_hsla.x, red_hsla.x, len), 1.0f, 0.5f, blue_hsla.w});
        }

        glBindBuffer(GL_ARRAY_BUFFER, this->instance_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BasicShaderInstanced::InstanceData) * instance_data.size(), instance_data.data(), GL_STATIC_DRAW);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        instanced_shader.Bind();
        instanced_shader.SetViewMatrix(view_mat);
        instanced_shader.SetProjectionMatrix(proj_mat);
        instanced_shader.SetModelMatrix(glm::mat4(1.0f));

        instanced_shader.SetTexture(white_texture.id);
        glBindVertexArray(this->base_mesh.vao);
        glDrawElementsInstanced(GL_TRIANGLES, this->base_mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr, instance_data.size());
    }
    virtual void KeyCallback(int key, int scancode, int action, int mods) override {
    }

    Mesh base_mesh;
    GLuint instance_buffer;
    BasicShaderInstanced instanced_shader;
    std::vector<glm::vec3> particle_positions;
    std::vector<glm::vec3> particle_velocities;
    std::vector<glm::vec3> predicted_positions;
    std::vector<Density> particle_densities;

    glm::vec3 gravity = {0.0f, -9.0f, 0.0f};
    float target_density = 400.0f;
    float pressure_multiplier = 50.0f;
    float near_pressure_multiplier = 7.0f;
    float smoothing_radius = 0.2f;
    float viscosity_strength = 0.05f;
    float collision_damping = 0.95f;

    glm::vec3 radius;
    BoundingBox bb;
};
struct LightShafts : public Scene {
    LightShafts() : sponza(LoadGLTFLoadData(TranslateRelativePath("../../assets/sponza.glb").c_str())) {
        this->occlusion_map = CreateRenderTexture(1024, 1024);
    }
    ~LightShafts() {
    }
    virtual void Draw(const glm::mat4& proj_mat, const glm::mat4& view_mat, const BasicShader& basic_shader, uint32_t width, uint32_t height) {
        BoundingBox full_bb = {
            .min = {FLT_MAX, FLT_MAX, FLT_MAX},
            .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX},
        };
        const glm::vec3 sponza_scale = glm::vec3(0.01f);
        for(uint32_t i = 0; i < sponza.meshes.size(); ++i) {
            const BoundingBox& bb = sponza.meshes.at(i).mesh.bb;
            full_bb.min = glm::min(bb.min * sponza_scale, full_bb.min);
            full_bb.max = glm::max(bb.max * sponza_scale, full_bb.max);
        }

        glm::vec3 cam_pos = glm::vec3(glm::inverse(view_mat)[3]);

        const glm::vec3 light_dir = glm::normalize(glm::vec3(0.0f, -4.0f, 1.0f));
        constexpr float distance = 1000.0f;
        const glm::vec3 light_pos = cam_pos + distance * -light_dir;
        const glm::mat4 sponza_mat = glm::scale(glm::mat4(1.0f), sponza_scale);

        // draw things that occlude the light source
        {
            glBindFramebuffer(GL_FRAMEBUFFER, occlusion_map.framebuffer_id);
            glViewport(0, 0, occlusion_map.width, occlusion_map.height);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClearDepthf(1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            color_shader.Bind();
            color_shader.SetViewProjMatrix(proj_mat * view_mat);
            color_shader.SetColor(glm::vec4(0.0f));
            
            color_shader.SetModelMatrix(sponza_mat);
            for(uint32_t i = 0; i < sponza.meshes.size(); ++i) {
                const auto& mesh = sponza.meshes.at(i);
                glBindVertexArray(mesh.mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
            }
        }

        glm::vec4 light_screen_pos = proj_mat * view_mat * glm::vec4(light_pos, 1.0f);
        light_screen_pos /= light_screen_pos.w;
        light_screen_pos = 0.5f * (light_screen_pos + glm::vec4(1.0f));



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
        basic_shader.SetModelMatrix(sponza_mat);
        
        for(uint32_t i = 0; i < sponza.meshes.size(); ++i) {
            const auto& mesh = sponza.meshes.at(i);
            basic_shader.SetTexture(sponza.textures.at(sponza.materials.at(mesh.material_idx).base_color.idx).id);
            glBindVertexArray(mesh.mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.mesh.triangle_count * 3, GL_UNSIGNED_INT, nullptr);
        }

        light_ray_shader.Bind();
        light_ray_shader.SetDensity(1.0f);
        light_ray_shader.SetWeight(0.01f);
        light_ray_shader.SetDecay(1.0f);
        light_ray_shader.SetExposure(1.0f);
        light_ray_shader.SetNumSamples(100);
        light_ray_shader.SetLightScreenSpacePos(light_screen_pos);
        light_ray_shader.SetOcclusionTexture(occlusion_map.colorbuffer_ids[0]);

        static bool once = true;
        static GLuint full_screen_vao = INVALID_GL_HANDLE;
        static GLuint full_screen_vbo = INVALID_GL_HANDLE;
        if(once) {
            static glm::vec2 positions[] = {
                {-1.0f,  1.0f},
                {-1.0f, -1.0f},
                { 1.0f, -1.0f},
                {-1.0f,  1.0f},
                { 1.0f, -1.0f},
                { 1.0f,  1.0f},
            };

            glCreateVertexArrays(1, &full_screen_vao);
            glBindVertexArray(full_screen_vao);
            glGenBuffers(1, &full_screen_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, full_screen_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * ARRSIZE(positions), positions, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
            once = false;
        }
        // additive blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        glBindVertexArray(full_screen_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
    }
    virtual void Update(float dt) {
        
    }
    virtual void KeyCallback(int key, int scancode, int action, int mods) {
    }

    RenderTexture occlusion_map;
    ColorShader color_shader;
    LightRayShader light_ray_shader;
    GLTFLoadData sponza;
};



enum CurrentActiveScene {
    CS_RaytraceCubeScene,
    CS_IntersectionCubeScene,
    CS_DecimationScene,
    CS_VolumetricFog,
    CS_CascadedShadowMap,
    CS_FluidSim,
    CS_LightShafts,
};
static CurrentActiveScene active_scene = CS_LightShafts;
Scene* scene = nullptr;

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(scene) {
        scene->KeyCallback(key, scancode, action, mods);
    }
}
static std::string test_string;
static void CharacterCallback(GLFWwindow* window, unsigned int codepoint) {
    utf8::append(codepoint, test_string);
}


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

    if(active_scene == CurrentActiveScene::CS_IntersectionCubeScene) {
        scene = new IntersectionCubeScene(win_width, win_height);
    }
    else if(active_scene == CurrentActiveScene::CS_RaytraceCubeScene) {
        scene = new RaytraceCubeScene();
    }
    else if(active_scene == CurrentActiveScene::CS_DecimationScene) {
        scene = new DecimationScene(win_width, win_height);
    }
    else if(active_scene == CurrentActiveScene::CS_VolumetricFog) {
        scene = new VolumetricFog(win_width, win_height);
    }
    else if(active_scene == CurrentActiveScene::CS_CascadedShadowMap) {
        scene = new CascadedShadowMap();
    }
    else if(active_scene == CurrentActiveScene::CS_FluidSim) {
        scene = new FluidSimScene(15, 15, 15);
    }
    else if(active_scene == CurrentActiveScene::CS_LightShafts) {
        scene = new LightShafts();
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
    

    auto start = std::chrono::high_resolution_clock::now();
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();


        auto now = std::chrono::high_resolution_clock::now();
        const float dt = std::chrono::duration<float>(now - start).count();
        start = now;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glfwGetWindowSize(window, &win_width, &win_height);
        const glm::vec2 win_size = {static_cast<float>(win_width), static_cast<float>(win_height)};
        proj = glm::perspective(FOV, win_size.x / win_size.y, NEAR_PLANE, FAR_PLANE);

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

        scene->Update(dt);
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
