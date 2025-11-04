#pragma once

#include "Vector4.h"
#include "Matrix4x4.h"
#include <cstdint>

/*LambertianReflectance*/

/// マテリアルを拡張する

struct Material {

    Vector4 color;

    int32_t enableLighting = false;

    int32_t hasTexture = true;

    // 0=Lightingなし, 1=Lambert, 2=HalfLambert(デフォルトはHalfLambertに設定)
    int32_t lightingMode = 2;

    /*UVTransform*/

    ///Alignmentを考慮して書き換え

    float padding;

    ///Materialの拡張

    Matrix4x4 uvTransform;

    /*PhongReflectionModel*/

    float shininess = 1.0f;

};