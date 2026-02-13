#pragma once

#include "src/Renderer/Model.h"
#include "src/Math/AssimpToGLMConvert.h"

#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Mc
{
// 定义最大的骨骼数量，用于着色器Uniform数组大小
#define MAX_BONES 100
// 每个顶点最多影响的骨骼数
#define MAX_BONE_INFLUENCE 4

    // 骨骼的运行时信息
    struct BoneInfo
    {
        // 骨骼的唯一运行时 ID (索引)，用于在 FinalBoneMatrices 数组中查找
        int id = -1;

        // 骨骼的偏移矩阵 (Inverse Bind Pose Matrix)
        // 作用: 将顶点从模型的局部空间转换到骨骼的局部空间
        glm::mat4 offsetMatrix = glm::mat4(1.0f);
    };

    class Model;


    struct KeyPosition
    {
        glm::vec3 position;
        float timeStamp;
    };

    struct KeyRotation
    {
        glm::quat orientation;
        float timeStamp;
    };

    struct KeyScale
    {
        glm::vec3 scale;
        float timeStamp;
    };

    class Bone
    {
    public:
        /*reads keyframes from aiNodeAnim*/
        Bone(const std::string &name, int ID, const aiNodeAnim *channel);

        /*interpolates  b/w positions,rotations & scaling keys based on the curren time of
        the animation and prepares the local transformation matrix by combining all keys
        tranformations*/
        void Update(float animationTime);

        glm::mat4 GetLocalTransform() { return m_LocalTransform; }
        std::string GetBoneName() const { return m_Name; }
        int GetBoneID() { return m_ID; }

        /* Gets the current index on mKeyPositions to interpolate to based on
        the current animation time*/
        int GetPositionIndex(float animationTime)
        {
            for (int index = 0; index < m_NumPositions - 1; ++index)
            {
                if (animationTime < m_Positions[index + 1].timeStamp)
                    return index;
            }
            assert(0);
        }

        /* Gets the current index on mKeyRotations to interpolate to based on the
        current animation time*/
        int GetRotationIndex(float animationTime)
        {
            for (int index = 0; index < m_NumRotations - 1; ++index)
            {
                if (animationTime < m_Rotations[index + 1].timeStamp)
                    return index;
            }
            assert(0);
        }

        /* Gets the current index on mKeyScalings to interpolate to based on the
        current animation time */
        int GetScaleIndex(float animationTime)
        {
            for (int index = 0; index < m_NumScalings - 1; ++index)
            {
                if (animationTime < m_Scales[index + 1].timeStamp)
                    return index;
            }
            assert(0);
        }

    private:
        /* Gets normalized value for Lerp & Slerp*/
        float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);

        /*figures out which position keys to interpolate b/w and performs the interpolation
        and returns the translation matrix*/
        glm::mat4 InterpolatePosition(float animationTime);

        /*figures out which rotations keys to interpolate b/w and performs the interpolation
        and returns the rotation matrix*/
        glm::mat4 InterpolateRotation(float animationTime);

        /*figures out which scaling keys to interpolate b/w and performs the interpolation
        and returns the scale matrix*/
        glm::mat4 InterpolateScaling(float animationTime);

    private:
        std::vector<KeyPosition> m_Positions;
        std::vector<KeyRotation> m_Rotations;
        std::vector<KeyScale> m_Scales;
        int m_NumPositions;
        int m_NumRotations;
        int m_NumScalings;

        glm::mat4 m_LocalTransform;
        std::string m_Name;
        int m_ID;
    };

    // ==========================================================================================

    struct AssimpNodeData
    {
        glm::mat4 transformation;
        std::string name;
        int childrenCount;
        std::vector<AssimpNodeData> children;
    };

    class Animation
    {
    public:
        Animation() = default;

        Animation(const aiScene *scene, glm::mat4 globalInverseTransform, std::map<std::string, BoneInfo> &boneInfoMap, int &boneCount);

        ~Animation()
        {
        }

        Bone *FindBone(const std::string &name);

        float GetTicksPerSecond() { return m_TicksPerSecond; }
        float GetDuration() { return m_Duration; }
        const AssimpNodeData &GetRootNode() { return m_RootNode; }
        const std::map<std::string, BoneInfo> &GetBoneIDMap() { return m_BoneInfoMap; }
        const glm::mat4 GetGlobalInverseTransform() { return m_GlobalInverseTransform; }

        static Ref<Animation> Create(const aiScene *scene, glm::mat4 globalInverseTransform, std::map<std::string, BoneInfo> &boneInfoMap, int &boneCount);

    private:
        void ReadMissingBones(const aiAnimation *animation, std::map<std::string, BoneInfo> &boneInfoMap, int &boneCount);
        void ReadHeirarchyData(AssimpNodeData &dest, const aiNode *src);

        float m_Duration;
        int m_TicksPerSecond;
        std::vector<Bone> m_Bones;
        AssimpNodeData m_RootNode;
        std::map<std::string, BoneInfo> m_BoneInfoMap;
        glm::mat4 m_GlobalInverseTransform;
    };

    class Animator
    {
    public:
        Animator(Animation* currentAnimation);

        void UpdateAnimation(float dt);
        void PlayAnimation(Animation *pAnimation);
        void CalculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform);
        float GetCurrentTime() { return m_CurrentTime; }
        float GetDuration() { return m_CurrentAnimation->GetDuration(); }
        std::vector<glm::mat4> GetFinalBoneMatrices() { return m_FinalBoneMatrices; }
        void SetProgress(float fraction);
        bool GetPaused() { return m_IsPaused; }
        void SetPaused(bool paused) { m_IsPaused = paused; }

        static Ref<Animator> Create(Animation *currentAnimation);

    private:
        std::vector<glm::mat4> m_FinalBoneMatrices;
        Animation *m_CurrentAnimation;
        float m_CurrentTime;
        float m_DeltaTime;

        float m_TimeScale;
        bool m_IsPaused;
    };
}