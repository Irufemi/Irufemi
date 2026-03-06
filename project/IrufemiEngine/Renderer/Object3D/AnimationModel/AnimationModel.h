#pragma once

#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Transform.h"
#include "Resource/Model/Data/AnimationTransform.h"
#include "Renderer/TransformationMatrix.h"
#include "Resource/Model/Data/Animation.h"
#include "Resource/Model/Data/NodeAnimation.h"
#include "Resource/Model/Data/ModelData.h"
#include "Resource/Model/Data/ObjModel.h"
#include "Resource/Model/Data/Skeleton.h"
#include "Resource/Model/Data/SkinCluster.h"
#include <d3d12.h>
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <wrl.h>

// 前方宣言
class Camera;
class IrufemiEngine;
class SphereRegion;
class Line3DRegion;
struct ManagedModel;


class AnimationModel {
public: // メンバ関数

    AnimationModel();
    ~AnimationModel();

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
    // 指定したインデックスのメッシュのマテリアルを取得(読み取り専用)
    const ObjMaterial* GetMaterial(size_t meshIndex) const;
    // 指定したインデックスのメッシュのマテリアルを取得(書き込み可能)
    ObjMaterial* GetMaterial(size_t meshIndex);


    static void SetIrufemiEngine(IrufemiEngine* engine) { engine_ = engine; }

private: // メンバ変数
    // 共有モデルデータ(CPU/GPU)
    std::shared_ptr<ManagedModel> managedModel_;

    // オブジェクト全体のTransform
    Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    TransformationMatrix transformationMatrix_{};

    // 変換行列用リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_;
    TransformationMatrix* transformationData_ = nullptr;

    // メッシュごとの変換行列リソース
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> meshTransformationResources_;
    std::vector<TransformationMatrix*> meshTransformationData_;
    std::map<std::string, Matrix4x4> nodeWorldMatrices_;

    Camera* camera_ = nullptr;

    static IrufemiEngine* engine_;

    Skeleton skeleton_;

    SkinCluster skinCluster_;

    // ノードアニメーション用の固有Matrix
    Matrix4x4 localMatrix_;

    Matrix4x4 worldMatrix_;

    Animation animation_;

    float animationTime_ = 0.0f;

    // --- 追加：関節表示用のインスタンス描画機構 ---
    std::unique_ptr<SphereRegion> jointSpheres_;
    std::unique_ptr<Line3DRegion> boneLines_;
};