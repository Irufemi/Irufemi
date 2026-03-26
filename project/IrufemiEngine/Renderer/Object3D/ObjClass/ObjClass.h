#pragma once
#include <d3d12.h>
#include <string>
#include "Application/camera/Camera.h"
#include "Renderer/TransformationMatrix.h"
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "Engine/Core/Math/Transform.h"
#include "Resource/Model/Data/ObjModel.h"
#include "Renderer/Object3D/Object3DResource.h"

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;
class ModelManager;
struct ManagedModel;
struct GpuMaterial;
struct Material;

//==========================
// objが配布されているサイト
// https://quaternius.com/
// 使用する場合はライセンスがCCOのものを利用する
// https://creativecommons.org/publicdomain/zero/1.0/deed.ja
//==========================

class ObjClass {
private:
    // 共有モデルデータ(CPU/GPU)
    std::shared_ptr<ManagedModel> managedModel_;

    // オブジェクト全体のTransform
    Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    TransformationMatrix transformationMatrix_{};
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // インスタンスカラー

    // 各メッシュ用リソース (マテリアルや頂点Viewを保持)
    std::vector<std::unique_ptr<Object3DResource>> meshResources_;

    // 共通の変換行列リソース (全メッシュで共有)
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_;
    TransformationMatrix* transformationData_ = nullptr;

    Camera* camera_ = nullptr;
    
    // 行列更新の最適化用
    bool isDirty_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};

    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    static ModelManager* modelManager_;

    // CPU側のマテリアルデータをGPUリソースへ転送する
    void UpdateMaterials();

public: //メンバ関数

    //デストラクタ
    ~ObjClass();
    //初期化
    void Initialize(Camera* camera, const std::string& filename = "plane.obj");
    void Update();
    void Draw();
    void Debug(const char* objName = " ");
    void DebugTab();

    // Transform 系ゲッター/セッター (オブジェクト全体のTransformを操作するように変更)
    const Vector3& GetPosition() const { return transform_.translate; }
    void SetPosition(const Vector3& position) { transform_.translate = position; isDirty_ = true; }

    const Vector3& GetRotate() const { return transform_.rotate; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; isDirty_ = true; }
    void SetRotateX(const float& rotate) { transform_.rotate.x = rotate; isDirty_ = true; }
    void SetRotateY(const float& rotate) { transform_.rotate.y = rotate; isDirty_ = true; }
    void SetRotateZ(const float& rotate) { transform_.rotate.z = rotate; isDirty_ = true; }

    // 拡縮
    const Vector3& GetScale() const { return transform_.scale; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; isDirty_ = true; }
    const Transform& GetTransform() const { return transform_; }
    void SetTransform(const Transform& transform) { transform_ = transform; isDirty_ = true; }
    const TransformationMatrix& GetTransformationMatrix() const { return transformationMatrix_; }
    void SetTransformationMatrix(const TransformationMatrix& transformationMatrix) { transformationMatrix_ = transformationMatrix; }

    // --- マテリアル関連のメソッド ---

    // モデルが持つメッシュ数を取得
    size_t GetMeshCount() const;

    // 指定したインデックスのメッシュのマテリアルを取得(読み取り専用)
    const ObjMaterial* GetMaterial(size_t meshIndex) const;

    // 指定したインデックスのメッシュのマテリアルを取得(書き込み可能)
    ObjMaterial* GetMaterial(size_t meshIndex);

    // すべてのメッシュのライティングを一括で設定
    void SetEnableLightingToAllMeshes(bool enable);

    // インスタンスのアルファ値を設定
    void SetAlpha(float alpha);

    // インスタンスの色を一括で設定
    void SetColor(const Vector4& color);

    // 描画用の変換行列リソースのGPUアドレスを取得
    D3D12_GPU_VIRTUAL_ADDRESS GetTransformationGpuAddress() const {
        return transformationResource_->GetGPUVirtualAddress();
    }

    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; }
};

