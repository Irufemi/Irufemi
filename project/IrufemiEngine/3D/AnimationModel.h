#pragma once

#include "math/Matrix4x4.h"
#include "math/AnimationTransform.h"
#include "math/TransformationMatrix.h"
#include "math/Animation.h"
#include "math/NodeAnimation.h"
#include "math/ModelData.h"
#include "math/Transform.h"
#include "math/ObjModel.h" // ObjMaterial と ObjModel を使うためにインクルード
#include <d3d12.h>
#include <string>
#include <cstdint>
#include <memory>
#include "wrl.h"

// 前方宣言
class Camera;
class TextureManager;
class DrawManager;
class DebugUI;
class ModelManager;
class AnimationManager;
struct ManagedModel;


class AnimationModel {
public: // メンバ関数

    void Initialize(Camera* camera, const std::string& filename);

    void Update();

    void Draw();

    void Debug(const char* objName = " ");

    // 描画用の変換行列リソースのGPUアドレスを取得
    D3D12_GPU_VIRTUAL_ADDRESS GetTransformationGpuAddress() const {
        return transformationResource_->GetGPUVirtualAddress();
    }

private: // メンバ関数(内部ヘルパ)

    void UpdateMaterials();

    void UpdateAnimation();

public: // ゲッター・セッター
    // 指定したインデックスのメッシュのマテリアルを取得（読み取り専用）
    const ObjMaterial* GetMaterial(size_t meshIndex) const;
    // 指定したインデックスのメッシュのマテリアルを取得（書き込み可能）
    ObjMaterial* GetMaterial(size_t meshIndex);


    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; }
    static void SetAnimationManager(AnimationManager* am) { animationManager_ = am; }

private: // メンバ変数
    // 共有モデルデータ（CPU/GPU）
    std::shared_ptr<ManagedModel> managedModel_;

    // オブジェクト全体のTransform
    Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    TransformationMatrix transformationMatrix_{};

    // 変換行列用リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_;
    TransformationMatrix* transformationData_ = nullptr;

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    static ModelManager* modelManager_;
    static AnimationManager* animationManager_;

    Matrix4x4 localMatrix_;

    Matrix4x4 worldMatrix_;

    Animation animation_;

    float animationTime_ = 0.0f;

};