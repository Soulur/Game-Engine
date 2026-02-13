#include "AssimpToGLMConvert.h"

namespace Mc
{
    namespace AssimpToGLM
    {
        glm::mat4 Convert(const aiMatrix4x4 &from)
        {
            glm::mat4 to;

            // 第一列
            to[0][0] = from.a1;
            to[1][0] = from.a2;
            to[2][0] = from.a3;
            to[3][0] = from.a4;
            // 第二列
            to[0][1] = from.b1;
            to[1][1] = from.b2;
            to[2][1] = from.b3;
            to[3][1] = from.b4;
            // 第三列
            to[0][2] = from.c1;
            to[1][2] = from.c2;
            to[2][2] = from.c3;
            to[3][2] = from.c4;
            // 第四列 (平移)
            to[0][3] = from.d1;
            to[1][3] = from.d2;
            to[2][3] = from.d3;
            to[3][3] = from.d4;

            return to;
        }

        glm::vec3 Convert(const aiVector3D &from)
        {
            return glm::vec3(from.x, from.y, from.z);
        }

        glm::quat Convert(const aiQuaternion &from)
        {
            return glm::quat(from.w, from.x, from.y, from.z);
        }
    }
}