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
        Model(std::string const &path);
        
        bool IsLoaded() const { return m_IsLoaded; }

        std::string GetPath() { return m_Path; }
        
        std::vector<Ref<Mesh>> GetMeshs() { return m_Meshes; }

    private:
        void LoadModel(const std::filesystem::path &path);

        void ProcessNode(aiNode *node, const aiScene *scene);
        Ref<Mesh> ProcessMesh(aiMesh *mesh, const aiScene *scene);

    public:
        static Ref<Model> Create(std::string const &path);
    private:
        std::string m_Path;
        std::vector<Ref<Mesh>> m_Meshes;

        std::filesystem::path m_Directory;
        bool m_IsLoaded = false;
    };
}