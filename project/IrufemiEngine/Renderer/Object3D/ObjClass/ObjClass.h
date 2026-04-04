#pragma once
#include <d3d12.h>
#include <string>
#include "../../../../Application/camera/Camera.h"
#include "../../TransformationMatrix.h"
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "../../../Engine/Core/Math/Transform.h"
#include "../../../Engine/Core/Math/Vector4.h"
#include "../../../Engine/Core/Math/Matrix4x4.h"
#include "../../../Resource/Model/Data/ObjModel.h"
#include "../Object3DResource.h"
#include "../../../Engine/Graphics/Data/Material.h"

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;
class ModelManager;
struct ManagedModel;
struct ObjMaterial;
struct Material;

//==========================
// objが配布されているサイト
// https://quaternius.com/
// 使用する場合はライセンスがCCOのものを利用する
// https://creativecommons.org/publicdomain/zero/1.0/deed.ja
//==========================

/**
 * @class ObjClass
 * @brief 3Dモデル（OBJ/GLTF等）のインスタンスを描画・管理するクラス
 * @details ModelManager から取得した共有モデルデータを参照し、個別の位置・回転・拡縮やマテリアル設定を保持します。
 */
class ObjClass {
private:
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

    /**
     * @brief CPU側のマテリアルデータをGPUリソースへ転送する（内部用）
     */
    void UpdateMaterials();

public: //メンバ関数

    /**
     * @brief デストラクタ
     */
    ~ObjClass();

    /**
     * @brief 初期化
     * @param[in] camera 使用するカメラ
     * @param[in] filename モデルファイル名（ModelManager経由でロード）
     */
    void Initialize(Camera* camera, const std::string& filename = "plane.obj");

    /**
     * @brief 更新処理
     * @details ワールド行列の計算と定数バッファへの転送を行います。
     */
    void Update();

    /**
     * @brief 描画コマンドの積み込み
     */
    void Draw();

    /**
     * @brief デバッグ用UIの表示
     */
    void Debug(const char* objName = " ");

    /**
     * @brief デバッグ用タブの表示
     */
    void DebugTab();

    /** @name Transform 操作 */
    ///@{
    const Vector3& GetPosition() const { return transform_.translate; }
    void SetPosition(const Vector3& position) { transform_.translate = position; isDirty_ = true; }

    const Vector3& GetRotate() const { return transform_.rotate; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; isDirty_ = true; }
    void SetRotateX(const float& rotate) { transform_.rotate.x = rotate; isDirty_ = true; }
    void SetRotateY(const float& rotate) { transform_.rotate.y = rotate; isDirty_ = true; }
    void SetRotateZ(const float& rotate) { transform_.rotate.z = rotate; isDirty_ = true; }

    const Vector3& GetScale() const { return transform_.scale; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; isDirty_ = true; }
    const Transform& GetTransform() const { return transform_; }
    void SetTransform(const Transform& transform) { transform_ = transform; isDirty_ = true; }
    ///@}

    /** @name 行列・計算結果の取得 */
    ///@{
    const TransformationMatrix& GetTransformationMatrix() const { return transformationMatrix_; }
    void SetTransformationMatrix(const TransformationMatrix& transformationMatrix) { transformationMatrix_ = transformationMatrix; }
    D3D12_GPU_VIRTUAL_ADDRESS GetTransformationGpuAddress() const {
        return transformationResource_->GetGPUVirtualAddress();
    }
    ///@}

    /** @name マテリアル・外観の操作 */
    ///@{
    /**
     * @brief モデルが持つメッシュ数を取得
     */
    size_t GetMeshCount() const;

    /**
     * @brief 指定したインデックスのメッシュのマテリアルを取得
     */
    const ObjMaterial* GetMaterial(size_t meshIndex) const;
    ObjMaterial* GetMaterial(size_t meshIndex);

    /**
     * @brief すべてのメッシュのライティングを一括で有効/無効化する
     */
    void SetEnableLightingToAllMeshes(bool enable);

    /**
     * @brief インスタンス全体のアルファ値を設定
     */
    void SetAlpha(float alpha);

    /**
     * @brief インスタンス全体の色を設定
     */
    void SetColor(const Vector4& color);
    void SetEnvironmentCoefficient(float coefficient) { environmentCoefficient_ = coefficient; isDirty_ = true; }
    void SetLightingModeOverride(int32_t mode) { lightingModeOverride_ = mode; isDirty_ = true; }
    void SetUseClampSamplerOverride(int32_t useClamp) { useClampSamplerOverride_ = useClamp; isDirty_ = true; }
    void SetEnableLightingOverride(int32_t enable) { enableLightingOverride_ = enable; isDirty_ = true; }
    ///@}

    /** @name 静的メンバ設定（エンジン内部用） */
    ///@{
    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; }
    ///@}
};

