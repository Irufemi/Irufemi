#pragma once

#include "math/Matrix4x4.h"
#include "math/AnimationTransform.h"
#include "math/TransformationMatrix.h"
#include "math/Animation.h"
#include "math/NodeAnimation.h"
#include "math/ModelData.h"
#include "source/D3D12ResourceUtil.h"
#include <d3d12.h>
#include <string>

// 前方宣言
class Camera;
class TextureManager;
class DrawManager;
class DebugUI;
class ModelManager;
struct ManagedModel;


class AnimationModel{
public: // メンバ関数

    void Initialize(Camera* camera, const std::string& directoryPath, const std::string& filename);

    void Update();

    void Draw();

    void Debug();

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数
private:
    // 共有モデルデータ（CPU/GPU）
    std::shared_ptr<ManagedModel> managedModel_;

    std::vector<std::unique_ptr<Texture>> textures_;
    // インスタンス固有リソース（Transform, Material等）
    std::vector<std::unique_ptr<D3D12ResourceUtil>> instanceResources_;

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    static ModelManager* modelManager_;

    // 共有モデルデータ（CPU/GPU）
    std::shared_ptr<ManagedModel> managedModel_;

    std::vector<std::unique_ptr<Texture>> textures_;
    // インスタンス固有リソース（Transform, Material等）
    std::vector<std::unique_ptr<D3D12ResourceUtil>> instanceResources_;

    Matrix4x4 localMatrix_;

    Matrix4x4 worldMatrix_;

    Animation animation_;

    NodeAnimation& rootNodeAnimation;

    float animationTime = 0.0f;

    Camera* camera_ = nullptr;

};

