#pragma once

#include "Vector3.h"
#include "Vector4.h"

struct SpotLight{
    //!< ライトの色
    Vector4 color;
    //!< ライトの位置
    Vector3 position;
    //! 輝度
    float intensity;
    //!< スポットライトの方向
    Vector3 direction;
    //!< ライトの届く最大距離
    float distance;
    //!<減衰率
    float decay;
    //!< スポットライトの余弦
    float cosAngle;
    
    float padding[2];
};