#pragma once

#include "src/Core/Base.h"
#include "src/Renderer/Shader.h"
#include "src/Renderer/Mesh.h"
#include "src/Renderer/Texture.h"
#include "src/Renderer/Material.h"
#include "src/Renderer/Animation.h"

#include <filesystem>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Mc
{   
    struct BoneChannel;
    struct BoneInfo;
    class Animation;
    class Animator;

    class Model
    {
    public:
        // constructor, expects a filepath to a 3D model.
        Model(std::string const &path);
        
        bool IsLoaded() const { return m_IsLoaded; }

        std::string GetPath() { return m_Path; }
        
        std::vector<Ref<Mesh>> GetMeshs() { return m_Meshes; }

        glm::mat4 GetGlobalInverseTransform() { return m_GlobalInverseTransform; }
        std::map<std::string, BoneInfo> &GetBoneInfoMap() { return m_BoneInfoMap; };
        int &GetBoneCount() { return m_BoneCounter; }
        bool GetIsAnimation() { return m_IsAnimation; }
        Ref<Animator> GetAnimator() { return m_Animator; }

    private:
        void LoadModel(const std::filesystem::path &path);

        void ProcessNode(aiNode *node, const aiScene *scene);
        Ref<Mesh> ProcessMesh(aiMesh *mesh, const aiScene *scene);

        // 辅助函数：将 BoneID 和 Weight 存入 ModelVertex
        void AddBoneDataToVertex(ModelVertex &vertex, int boneID, float weight);
    public:
        static Ref<Model> Create(std::string const &path);
    private:
        std::string m_Path;
        std::vector<Ref<Mesh>> m_Meshes;

        std::filesystem::path m_Directory;
        bool m_IsLoaded = false;

        glm::mat4 m_GlobalInverseTransform;

        std::map<std::string, BoneInfo> m_BoneInfoMap;
        int m_BoneCounter = 0;

        bool m_IsAnimation = false;
        Ref<Animation> m_DanceAnimation;
        Ref<Animator> m_Animator;
    };
}