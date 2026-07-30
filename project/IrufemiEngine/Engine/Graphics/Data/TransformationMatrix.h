#pragma once

#include "../../Core/Math/Matrix4x4.h"

/**
 * @struct TransformationMatrix
 * @brief 3Dオブジェクトの変換行列をGPUへ送るための構造体
 */
struct TransformationMatrix {
    Irufemi::Matrix4x4 WVP;
    Irufemi::Matrix4x4 world;
    Irufemi::Matrix4x4 WorldInverseTranspose;
};
