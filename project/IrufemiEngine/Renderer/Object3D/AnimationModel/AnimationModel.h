#pragma once

#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Transform.h"
#include "Resource/Model/Data/AnimationTransform.h"
#include "Renderer/TransformationMatrix.h"
#include "../../../Engine/Core/Math/Vector2.h"
#include "../../../Engine/Core/Math/Vector3.h"
#include "../../../Engine/Core/Math/Vector4.h"
#include "../../../Engine/Core/Math/Transform.h"
#include "Engine/Graphics/DirectX/ConstantBuffer.h"
#include "Resource/Model/Data/Animation.h"
#include "Resource/Model/Data/NodeAnimation.h"
#include "Resource/Model/Data/ModelData.h"
#include "Resource/Model/Data/ObjModel.h"
#include "Resource/Model/Data/Skeleton.h"
#include "Resource/Model/Data/SkinCluster.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Engine/Graphics/Data/Material.h"
#include "Engine/Manager/IComputeTask.h"
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
struct ObjMaterial; 
struct Material;


class AnimationModel : public IComputeTask {
public: // メンバ関数

    AnimationModel();
    ~AnimationModel();

    void DispatchCompute() override;

    void Initialize(Camera* camera, const std::string& filename);

    void Update();

    void Draw();

    void Debug(const char* objName = " ");

    // 描画用の変換行列リソースのGPUアドレスを取得
    D3D12_GPU_VIRTUAL_ADDRESS GetTransformationGpuAddress() const {
        return transformationBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }

private: // メンバ関数(内部ヘルパ)

    void UpdateMaterials();

    void UpdateAnimation();

    void InitializeResources();

public: // ゲッター・セッター
    // 指定したインデックスのメッシュのマテリアルを取得(読み取り専用)
    const ObjMaterial* GetMaterial(size_t meshIndex) const;
    // 指定したインデックスのメッシュのマテリアルを取得(書き込み可能)
    ObjMaterial* GetMaterial(size_t meshIndex);

    void SetColor(const Vector4& color) { color_ = color; isDirty_ = true; }
    const Vector4& GetColor() const { return color_; }

    void SetTranslate(const Vector3& translate) { transform_.translate = translate; isDirty_ = true; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; isDirty_ = true; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; isDirty_ = true; }
    void SetTransform(const Transform& transform) { transform_ = transform; isDirty_ = true; }
    const Transform& GetTransform() const { return transform_; }

    void SetEnvironmentCoefficient(float coefficient) { environmentCoefficient_ = coefficient; isDirty_ = true; }
    float GetEnvironmentCoefficient() const { return environmentCoefficient_; }

    void SetLightingModeOverride(int32_t mode) { lightingModeOverride_ = mode; isDirty_ = true; }
    void SetUseClampSamplerOverride(int32_t useClamp) { useClampSamplerOverride_ = useClamp; isDirty_ = true; }
    void SetEnableLightingOverride(int32_t enable) { enableLightingOverride_ = enable; isDirty_ = true; }


    static void SetIrufemiEngine(IrufemiEngine* engine) { engine_ = engine; }
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }

private: // メンバ変数
    // 共有モデルデータ(CPU/GPU)
    std::shared_ptr<ManagedModel> managedModel_;

    // オブジェクト全体のTransform
    Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    TransformationMatrix transformationMatrix_{};
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // インスタンスカラー
    float environmentCoefficient_ = 1.0f; // インスタンス環境マップ係数
    int32_t lightingModeOverride_ = -1; // -1:使用しない, 0以上:上書き
    int32_t useClampSamplerOverride_ = -1; // -1:使用しない, 0以上:上書き
    int32_t enableLightingOverride_ = -1; // -1:使用しない, 0以上:上書き

    // --- 描画リソース (新アーキテクチャ) ---
    std::vector<std::unique_ptr<Object3DResource>> meshResources_;

    // 変換行列用リソース (全メッシュ共有)
    ConstantBuffer<TransformationMatrix> transformationBuffer_;
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

    // 行列更新の最適化用
    bool isDirty_ = true;
    bool isCullingEnabled_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};

    int32_t dirtyFramesLeft_ = kMaxFramesInFlight;
    uint32_t lastSyncedFrameIndex_ = UINT32_MAX;
    void MakeDirty() { dirtyFramesLeft_ = kMaxFramesInFlight; }
    void SyncIfDirty();

    std::string filename_;
};