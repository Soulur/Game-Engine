#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <assimp/scene.h>

namespace Mc
{
    namespace AssimpToGLM
    {
        glm::mat4 Convert(const aiMatrix4x4 &from);
        glm::vec3 Convert(const aiVector3D &from);
        glm::quat Convert(const aiQuaternion &from);
    }
}