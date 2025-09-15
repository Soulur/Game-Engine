#include "model.h"

#include "src/Renderer/Manager/TextureManager.h"
#include "src/Renderer/Manager/MaterialManager.h"
#include "src/Renderer/Manager/MeshManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Mc
{
    Model::Model(std::string const &path)
        : m_Path(path)
    {
        LoadModel(path);
    }

    void Model::LoadAllSceneMaterials(const aiScene *scene)
    {
        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            aiMaterial *aiMat = scene->mMaterials[i];
            if (aiMat)
            {
                Ref<Material> material = MaterialManager::Get().AddMaterial();

                aiColor3D color(0.f, 0.f, 0.f);
                float value = 0.0f; // 用于 Shininess, Opacity, Refracti

                aiString name;
                if (AI_SUCCESS == aiMat->Get(AI_MATKEY_NAME, name)) material->SetName(name.C_Str()); // 如果 Material 有 Name 成员

                // **PBR 属性映射（关键重构点）**
                // PBR 属性的映射通常是启发式的，没有完美的1对1关系
                // 例如：
                // Albedo (基色): 通常来自 Diffuse Color 或 Diffuse Map
                if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color))
                    material->SetAlbedo(glm::vec4(color.r, color.g, color.b, 1.0f));
                else // 如果没有漫反射颜色，可能默认白色
                    material->SetAlbedo(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

                // Roughness (粗糙度): 通常从 Shininess 映射，或专用贴图
                // 传统光泽度(Shininess)与粗糙度(Roughness)呈反比关系
                if (AI_SUCCESS == aiMat->Get(AI_MATKEY_SHININESS, value))
                {
                    // 粗略映射：shininess 越大，roughness 越小
                    // 具体公式可能更复杂，例如 roughness = 1.0 - sqrt(shininess / max_shininess)
                    material->SetRoughness(glm::clamp(1.0f - (value / 256.0f), 0.0f, 1.0f)); // 假设max shininess是256
                }
                else
                    material->SetRoughness(0.5f); // 默认粗糙度

                // Metallic (金属度): 通常为0（非金属）或1（金属），或来自专用贴图
                // 传统材质通常没有金属度概念。这里可能根据SpecularColor的亮度或材质名称进行猜测
                if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color))
                {
                    // 粗略判断：如果高光颜色很亮且是灰度，可能倾向于金属
                    // 真实PBR会使用专门的Metallic贴图
                    float specBrightness = (color.r + color.g + color.b) / 3.0f;
                    material->SetMetallic(specBrightness > 0.8f ? 1.0f : 0.0f); // 简单粗暴的判断
                }
                else
                    material->SetMetallic(0.0f); // 默认非金属

                // AO (环境光遮蔽):
                if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_AMBIENT, color))
                    material->SetAO((color.r + color.g + color.b) / 3.0f); // 粗略地将环境光颜色作为AO

                // Emissive (自发光):
                if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, color))
                    material->SetEmissive(glm::vec3(color.r, color.g, color.b));

                // --------------------- 纹理加载 ---------------------
                // 使用新的 SetTexture 方法，它会通过 TextureManager 获取纹理
                aiString path;

                // Albedo Map (从 DIFFUSE 纹理获取)
                if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &path))
                    material->SetTexture(TextureType::Albedo, m_Directory.string() + path.C_Str());

                // Normal Map (从 NORMALS 或 HEIGHT 纹理获取)
                if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_NORMALS, 0, &path)) // 优先 NORMALS
                    material->SetTexture(TextureType::Normal, m_Directory.string() + path.C_Str());
                else if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_HEIGHT, 0, &path)) // 其次 HEIGHT
                    material->SetTexture(TextureType::Normal, m_Directory.string() + path.C_Str());

                if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_SPECULAR, 0, &path))
                {
                    // 假设高光贴图用作 Metallic 贴图
                    material->SetTexture(TextureType::Metallic, m_Directory.string() + path.C_Str());
                    // 或者你可能需要加载它，然后根据颜色值生成 roughness 或 metallic
                }

                // Emissive Map
                if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_EMISSIVE, 0, &path))
                    material->SetTexture(TextureType::Emissive, m_Directory.string() + path.C_Str());

                // Ambient Occlusion Map (Assimp 5.x 支持)
                if (AI_SUCCESS == aiMat->GetTexture(aiTextureType_AMBIENT, 0, &path)) // fallback to ambient if AO not present
                    material->SetTexture(TextureType::AmbientOcclusion, m_Directory.string() + path.C_Str());

                m_Materials.push_back(material);

                LOG_CORE_INFO("{0}", material->GetName());
            }
        }
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

        // Load
        LoadAllSceneMaterials(scene);
        ProcessNode(scene->mRootNode, scene);

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

        // Face
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; ++j)
                indices.push_back(face.mIndices[j]);
        }

        return Mesh::Create(name, vertices, indices);
    }

    Ref<Model> Model::Create(std::string const &path)
    {
        return CreateRef<Model>(path);
    }
}
