#pragma once

#include<glm/glm.hpp>

// Estructura contenedora para la información del hueso
struct BoneInfo
{
    /* id es el índice en finalBoneMatrices */
    int id;

    /* offset matrix transforma el vértice del espacio local del modelo al espacio del hueso */
    glm::mat4 offset;
};