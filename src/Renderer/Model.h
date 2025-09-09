#pragma once

#include "src/Core/Base.h"
#include "src/Renderer/Shader.h"
#include "src/Renderer/Mesh.h"
#include "src/Renderer/Texture.h"
#include "src/Renderer/Material.h"

#include <filesystem>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Mc
{
    class Model
    {
    public:
        // constructor, expects a filepath to a 3D model.
        Model(std::string const &path, bool FlipUV = false, bool gamma = false);
        
        void Draw(Ref<Shader> &shader)
        {
            shader->SetBool("u_FlipUV", m_FlipUV);
            for (auto &mesh : m_Meshes)
                mesh->Draw(shader);
        }

        void DrawInstanced(Ref<Shader> &shader, uint32_t instanceCount)
        {
            shader->SetBool("u_FlipUV", m_FlipUV);
            for (auto &mesh : m_Meshes)
                mesh->DrawInstanced(shader, instanceCount);
        }

        bool IsLoaded() const { return m_IsLoaded; }

        std::string GetPath() { return m_Path; }
        
        std::vector<Ref<Mesh>> GetMeshs() { return m_Meshes; }
        std::vector<Ref<Material>> GetMaterials()  { return m_Materials; }

        void SetFlipUV(bool flipuv) { m_FlipUV = flipuv; }
        bool GetFlipUV() { return m_FlipUV; }
    private:
        void LoadAllSceneMaterials(const aiScene *scene);

        void LoadModel(const std::filesystem::path &path, bool FlipUV = false);

        void ProcessNode(aiNode *node, const aiScene *scene);
        Ref<Mesh> ProcessMesh(aiMesh *mesh, const aiScene *scene);

    public:
        static Ref<Model> Create(std::string const &path, bool FlipUV = false, bool gamma = false);
    private:
        std::string m_Path;
        std::vector<Ref<Mesh>> m_Meshes;

        std::vector<Ref<Material>> m_Materials;

        bool m_IsLoaded = false;
        std::filesystem::path m_Directory;
        bool m_FlipUV;
        bool gammaCorrection;
    };
}