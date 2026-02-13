#include "model.h"

#include "src/Renderer/Manager/TextureManager.h"
#include "src/Renderer/Manager/MaterialManager.h"
#include "src/Renderer/Manager/MeshManager.h"
#include "src/Math/AssimpToGLMConvert.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Mc
{
    // --- 数据结构定义 ---

    // 形变目标数据结构：存储相对于基础网格的偏移量
    struct MorphTargetData
    {
        std::vector<aiVector3D> positionOffsets;
        std::vector<aiVector3D> normalOffsets;
        // ... 其他可选偏移量 (切线等)
    };

    // 动画关键帧结构：存储形变目标索引和时间
    struct MorphKey
    {
        double time;              // 关键帧时间
        unsigned int targetIndex; // 目标 aiAnimMesh 的索引
    };

    // 形变动画通道结构
    struct MorphChannel
    {
        std::string meshName;
        std::vector<MorphKey> keys;
    };

    Model::Model(std::string const &path)
        : m_Path(path)
    {
        LoadModel(path);
    }

    void Model::LoadModel(const std::filesystem::path &path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;

        const aiScene *scene;
        scene = importer.ReadFile(path.string(),
                                  aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
                                      aiProcess_CalcTangentSpace | aiProcess_FlipWindingOrder);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            LOG_CORE_ERROR("ERROR::ASSIMP:: {0}", importer.GetErrorString());
            return;
        }
        // m_Directory = path.string().substr(0, path.string().find_last_of('/'));
        m_Directory = path.parent_path().string();

        m_GlobalInverseTransform = glm::inverse(AssimpToGLM::Convert(scene->mRootNode->mTransformation));

        // Load
        ProcessNode(scene->mRootNode, scene);

        // Load Animation
        if (scene->mNumAnimations > 0 && scene->mAnimations != nullptr)
        {
            m_IsAnimation = true;
            m_DanceAnimation = Animation::Create(scene, m_GlobalInverseTransform, m_BoneInfoMap, m_BoneCounter);
            m_Animator = Animator::Create(m_DanceAnimation.get());
        }
        

        m_IsLoaded = true;
        LOG_CORE_INFO("Loaded true {0}", path);
    }

    void Model::ProcessNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
            m_Meshes.push_back(ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene));
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
           ProcessNode(node->mChildren[i], scene);
    }

    Ref<Mesh> Model::ProcessMesh(aiMesh *mesh, const aiScene *scene)
    {
        std::vector<ModelVertex> vertices;
        std::vector<uint32_t> indices;

        std::string name = mesh->mName.C_Str();

        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            ModelVertex node;
            // position
            aiVector3D pos = mesh->mVertices[i];
            node.Position = glm::vec3(pos.x, pos.y, pos.z);
            // normal
            if (mesh->HasNormals())
            {
                aiVector3D normal = mesh->mNormals[i];
                node.Normal = glm::vec3(normal.x, normal.y, normal.z);
            }
            if (mesh->HasTextureCoords(0))
            {
                // texCoord
                aiVector3D texCoord = mesh->mTextureCoords[0][i];
                node.TexCoords = glm::vec2(texCoord.x, texCoord.y);
                // tangents
                aiVector3D tangents = mesh->mTangents[i];
                node.Tangent = glm::vec3(tangents.x, tangents.y, tangents.z);
                // bitangent
                aiVector3D bitangent = mesh->mBitangents[i];
                node.Bitangent = glm::vec3(bitangent.x, bitangent.y, bitangent.z);
            }
            else
            {
                node.TexCoords = glm::vec2(0.0f);
                node.Tangent = glm::vec3(0.0f);
                node.Bitangent = glm::vec3(0.0f);
            }
            vertices.push_back(node);
        }

        // --- 骨骼权重提取 (新增) ---
        if (mesh->HasBones())
        {
            for (unsigned int i = 0; i < mesh->mNumBones; ++i)
            {
                aiBone *bone = mesh->mBones[i];
                std::string boneName = bone->mName.C_Str();
                int boneID = -1;

                // 1. 记录骨骼信息 (如果骨骼是第一次出现)
                if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
                {
                    BoneInfo newBone;
                    newBone.id = m_BoneCounter;
                    newBone.offsetMatrix = AssimpToGLM::Convert(bone->mOffsetMatrix);
                    m_BoneInfoMap[boneName] = newBone;
                    
                    boneID = m_BoneCounter;
                    m_BoneCounter++;
                }
                else
                {
                    boneID = m_BoneInfoMap[boneName].id;
                }

                // 2. 将权重分配给顶点
                for (unsigned int j = 0; j < bone->mNumWeights; ++j)
                {
                    aiVertexWeight weight = bone->mWeights[j];
                    unsigned int vertexID = weight.mVertexId;
                    float boneWeight = weight.mWeight;

                    // 使用辅助函数来处理权重分配和 MAX_BONE_INFLUENCE 限制
                    AddBoneDataToVertex(vertices[vertexID], boneID, boneWeight);
                }
            }
        }

        // Face
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; ++j)
                indices.push_back(face.mIndices[j]);
        }

        return Mesh::Create(name, vertices, indices);
    }

    void Model::AddBoneDataToVertex(ModelVertex &vertex, int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            if (vertex.BoneIDs[i] == -1) // 找到第一个空的槽位
            {
                vertex.BoneIDs[i] = boneID;
                vertex.Weights[i] = weight;
                return;
            }
        }
    }

    Ref<Model> Model::Create(std::string const &path)
    {
        return CreateRef<Model>(path);
    }
}
