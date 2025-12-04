#pragma once

#define MT4_01_02 1

#include "scene/IScene.h"

#include "audio/Bgm.h"
#include "audio/Se.h"
#include "math/shape/LinePrimitive.h"
#include "2D/Sprite.h"
#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "3D/CylinderClass.h"
#include "math/Matrix4x4.h"
#include <memory>
#include <vector>

/// <summary>
/// MT4
/// </summary>
class MT4 : public IScene {

public:
    
    // 課題で追加した関数

    /// <summary>
    /// 任意軸回転行列の作成関数
    /// </summary>
    /// <param name="axis"></param>
    /// <param name="angle"></param>
    /// <returns></returns>
    static Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle);

    /// <summary>
    /// ある方向からある方向への回転
    /// </summary>
    /// <param name="from"></param>
    /// <param name="to"></param>
    /// <returns></returns>
    static Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to);

private: // 課題で追加した変数

#ifdef MT4_01_01

    Vector3 axis;

    float angle;

    Matrix4x4 rotateMatrix;

#endif

#ifdef MT4_01_02

    Vector3 from0;

    Vector3 to0;

    Vector3 from1;

    Vector3 to1;

    Matrix4x4 rotateMatrix0;

    Matrix4x4 rotateMatrix1;

    Matrix4x4 rotateMatrix2;

#endif




public: // メンバ関数
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ変数
    IrufemiEngine* engine_ = nullptr;

    std::unique_ptr<Camera> camera_ = nullptr;

    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;
    bool debugMode = false;

    std::unique_ptr<PointLightClass> pointLight_ = nullptr;
    std::unique_ptr<SpotLightClass> spotLight_ = nullptr;
};
