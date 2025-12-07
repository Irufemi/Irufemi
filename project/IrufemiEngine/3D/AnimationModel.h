#pragma once

#include "math/Matrix4x4.h"
#include "math/AnimationTransform.h"
#include "math/TransformationMatrix.h"
#include "math/Animation.h"
#include "math/ModelData.h"
#include <string>

class Camera;

class AnimationModel{
public: // メンバ関数

    void Initialize(Camera* camera, const std::string& directoryPath, const std::string& filename);

    void Update();

    void Draw();

    void Debug();

private: // メンバ変数

    AnimationTrasform transform_;

    Matrix4x4 localMatrix_;

    Matrix4x4 worldMatrix_;

    TransformationMatrix* transformData_;

    Animation animation_;

    ModelData model_;

    float animationTime = 0.0f;

    Camera* camera_ = nullptr;

};

