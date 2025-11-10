#pragma once

#include "Vector3.h"
#include "Vector4.h"

/*LambertianReflectance*/

struct DirectionalLight {
    //!< ライトの色
    Vector4 color;
    //!< ライトの向き
    Vector3 direction;
    //!< 輝度
    float intensity;
};