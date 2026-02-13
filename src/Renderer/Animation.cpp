#include "Animation.h"
#include "src/Math/AssimpToGLMConvert.h"

namespace Mc
{

    Bone::Bone(const std::string &name, int ID, const aiNodeAnim *channel)
        : m_Name(name), m_ID(ID), m_LocalTransform(1.0f)
    {
        m_NumPositions = channel->mNumPositionKeys;

        for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
        {
            aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
            float timeStamp = channel->mPositionKeys[positionIndex].mTime;
            KeyPosition data;
            data.position = AssimpToGLM::Convert(aiPosition);
            data.timeStamp = timeStamp;
            m_Positions.push_back(data);
        }

        m_NumRotations = channel->mNumRotationKeys;
        for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
        {
            aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
            float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
            KeyRotation data;
            data.orientation = AssimpToGLM::Convert(aiOrientation);
            data.timeStamp = timeStamp;
            m_Rotations.push_back(data);
        }

        m_NumScalings = channel->mNumScalingKeys;
        for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
        {
            aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
            float timeStamp = channel->mScalingKeys[keyIndex].mTime;
            KeyScale data;
            data.scale = AssimpToGLM::Convert(scale);
            data.timeStamp = timeStamp;
            m_Scales.push_back(data);
        }
    }

    void Bone::Update(float animationTime)
    {
        glm::mat4 translation = InterpolatePosition(animationTime);
        glm::mat4 rotation = InterpolateRotation(animationTime);
        glm::mat4 scale = InterpolateScaling(animationTime);
        m_LocalTransform = translation * rotation * scale;
    }

    float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
    {
        float scaleFactor = 0.0f;
        float midWayLength = animationTime - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        scaleFactor = midWayLength / framesDiff;
        return scaleFactor;
    }

    glm::mat4 Bone::InterpolatePosition(float animationTime)
    {
        if (1 == m_NumPositions)
            return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

        int p0Index = GetPositionIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp,
                                           m_Positions[p1Index].timeStamp, animationTime);
        glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position,
                                           m_Positions[p1Index].position, scaleFactor);
        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    glm::mat4 Bone::InterpolateRotation(float animationTime)
    {
        if (1 == m_NumRotations)
        {
            auto rotation = glm::normalize(m_Rotations[0].orientation);
            return glm::toMat4(rotation);
        }

        int p0Index = GetRotationIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp,
                                           m_Rotations[p1Index].timeStamp, animationTime);
        glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation,
                                             m_Rotations[p1Index].orientation, scaleFactor);
        finalRotation = glm::normalize(finalRotation);
        return glm::toMat4(finalRotation);
    }

    glm::mat4 Bone::InterpolateScaling(float animationTime)
    {
        if (1 == m_NumScalings)
            return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

        int p0Index = GetScaleIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp,
                                           m_Scales[p1Index].timeStamp, animationTime);
        glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale, scaleFactor);
        return glm::scale(glm::mat4(1.0f), finalScale);
    }

    Animation::Animation(const aiScene *scene, glm::mat4 globalInverseTransform, std::map<std::string, BoneInfo> &boneInfoMap, int &boneCount)
    {
        aiAnimation *animation = scene->mAnimations[0];
        m_Duration = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond;
        m_GlobalInverseTransform = globalInverseTransform;
        ReadHeirarchyData(m_RootNode, scene->mRootNode);
        ReadMissingBones(animation, boneInfoMap, boneCount);
    }

    Bone *Animation::FindBone(const std::string &name)
    {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
                                 [&](const Bone &Bone)
                                 {
                                     return Bone.GetBoneName() == name;
                                 });
        if (iter == m_Bones.end())
            return nullptr;
        else
            return &(*iter);
    }

    Ref<Animation> Animation::Create(const aiScene *scene, glm::mat4 globalInverseTransform, std::map<std::string, BoneInfo> &boneInfoMap, int &boneCount)
    {
        return CreateRef<Animation>(scene, globalInverseTransform, boneInfoMap, boneCount);
    }

    void Animation::ReadMissingBones(const aiAnimation *animation, std::map<std::string, BoneInfo> &boneInfoMap, int &boneCount)
    {
        int size = animation->mNumChannels;

        // reading channels(bones engaged in an animation and their keyframes)
        for (int i = 0; i < size; i++)
        {
            auto channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.data;

            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                boneInfoMap[boneName].id = boneCount;
                boneCount++;
            }
            m_Bones.push_back(Bone(channel->mNodeName.data,
                                   boneInfoMap[channel->mNodeName.data].id, channel));
        }

        m_BoneInfoMap = boneInfoMap;
    }

    void Animation::ReadHeirarchyData(AssimpNodeData &dest, const aiNode *src)
    {
        assert(src);

        dest.name = src->mName.data;
        dest.transformation = AssimpToGLM::Convert(src->mTransformation);
        dest.childrenCount = src->mNumChildren;

        for (int i = 0; i < src->mNumChildren; i++)
        {
            AssimpNodeData newData;
            ReadHeirarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }

    Animator::Animator(Animation * currentAnimation)
    {
        m_CurrentTime = 0.0f;     // 当前处于第几个 Tick
        m_TimeScale = 1.0f;       // 播放速度（采样率）
        m_IsPaused = false;        // 是否暂停（手动拖动进度条时通常需要暂停）

        m_CurrentAnimation = currentAnimation;

        m_FinalBoneMatrices.reserve(MAX_BONES);

        for (int i = 0; i < MAX_BONES; i++)
            m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
    }

    void Animator::UpdateAnimation(float dt)
    {
        m_DeltaTime = dt;
        if (!m_IsPaused)
        {
            if (m_CurrentAnimation)
            {
                m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt * m_TimeScale;

                float duration = m_CurrentAnimation->GetDuration();
                if (m_CurrentTime > duration)
                    m_CurrentTime = fmod(m_CurrentTime, duration);
            }
        }
        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
    }

    void Animator::PlayAnimation(Animation *pAnimation)
    {
        m_CurrentAnimation = pAnimation;
        m_CurrentTime = 0.0f;
    }

    void Animator::CalculateBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform)
    {
        if (!node || node->childrenCount > 1000)
            return; // 超过正常范围说明内存损坏

        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;

        Bone *Bone = m_CurrentAnimation->FindBone(nodeName);

        if (Bone)
        {
            Bone->Update(m_CurrentTime);
            nodeTransform = Bone->GetLocalTransform();
        }

        glm::mat4 globalTransformation = parentTransform * nodeTransform;

        auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
        if (boneInfoMap.find(nodeName) != boneInfoMap.end())
        {
            int index = boneInfoMap[nodeName].id;
            glm::mat4 offset = boneInfoMap[nodeName].offsetMatrix;
            m_FinalBoneMatrices[index] = m_CurrentAnimation->GetGlobalInverseTransform() * globalTransformation * offset;
        }

        for (int i = 0; i < node->childrenCount; i++)
            CalculateBoneTransform(&node->children[i], globalTransformation);
    }

    void Animator::SetProgress(float fraction)
    {
        if (!m_CurrentAnimation || fraction >= 1.0f)
            return;

        float duration = m_CurrentAnimation->GetDuration();
        m_CurrentTime = std::clamp(fraction * duration, 0.0f, duration - 0.01f);

        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
    }

    Ref<Animator> Animator::Create(Animation *currentAnimation)
    {
        return CreateRef<Animator>(currentAnimation);
    }
}