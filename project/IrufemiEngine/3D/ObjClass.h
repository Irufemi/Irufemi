#pragma once
#include <d3d12.h>
#include <string>
#include "Application/camera/Camera.h"
#include "source/D3D12ResourceUtil.h"
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "math/Transform.h"
#include "math/ObjModel.h" // ObjMaterial と ObjModel を使うためにインクルード

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;
class ModelManager;
struct ManagedModel;

//==========================
// objが配布されているサイト
// https://quaternius.com/
// 使用する場合はライセンスがCCOのものを利用する
// https://creativecommons.org/publicdomain/zero/1.0/deed.ja
//==========================

class ObjClass {
private:
    // 共有モデルデータ（CPU/GPU）
    std::shared_ptr<ManagedModel> managedModel_;

    // オブジェクト全体のTransform
    Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    TransformationMatrix transformationMatrix_{};

    Camera* camera_ = nullptr;

    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    static ModelManager* modelManager_;

public: //メンバ関数

    //デストラクタ
    ~ObjClass() = default;
    //初期化
    void Initialize(Camera* camera, const std::string& filename = "plane.obj");
    void Update();
    void Draw();
    void Debug(const char* objName = " ");

    // Transform 系ゲッター/セッター (オブジェクト全体のTransformを操作するように変更)
    const Vector3& GetPosition() const { return transform_.translate; }
    void SetPosition(const Vector3& position) { transform_.translate = position; }

    const Vector3& GetRotate() const { return transform_.rotate; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
    void SetRotateX(const float& rotate) { transform_.rotate.x = rotate; }
    void SetRotateY(const float& rotate) { transform_.rotate.y = rotate; }
    void SetRotateZ(const float& rotate) { transform_.rotate.z = rotate; }

    // 拡縮
    const Vector3& GetScale() const { return transform_.scale; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; }
    const Transform& GetTransform() const { return transform_; }
    void SetTransform(const Transform& transform) { transform_ = transform; }
    const TransformationMatrix& GetTransformationMatrix() const { return transformationMatrix_; }
    void SetTransformationMatrix(const TransformationMatrix& transformationMatrix) { transformationMatrix_ = transformationMatrix; }

    // --- マテリアル関連のメソッド ---

    // モデルが持つメッシュ数を取得
    size_t GetMeshCount() const;

    // 指定したインデックスのメッシュのマテリアルを取得（読み取り専用）
    const ObjMaterial* GetMaterial(size_t meshIndex) const;

    // 指定したインデックスのメッシュのマテリアルを取得（書き込み可能）
    ObjMaterial* GetMaterial(size_t meshIndex);

    // すべてのメッシュのライティングを一括で設定
    void SetEnableLightingToAllMeshes(bool enable);

    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; }
};

