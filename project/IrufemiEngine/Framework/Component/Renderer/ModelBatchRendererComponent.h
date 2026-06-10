#pragma once
#include "../Component.h"
#include <memory>
#include <string>
#include "Engine/Core/Shape/Sphere.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Transform.h"

// 前方宣言
class ModelBatch;
class TransformComponent;

/**
 * @class ModelBatchRendererComponent
 * @brief インスタンシング（バッチ）描画を行うためのコンポーネント。
 *        毎フレーム外部から AddInstance() を呼び出してインスタンスを登録して使用します。
 */
class ModelBatchRendererComponent : public Component {
public:
    ModelBatchRendererComponent();
    ~ModelBatchRendererComponent() override;

    /**
     * @brief 初期化時にモデルファイル名を指定します
     * @param filename 読み込む .obj などのファイル名
     */
    void LoadModel(const std::string& filename);

    void Initialize() override;
    void Update() override;
    void Draw() override;
    
    bool CanUpdateInEditMode() const override { return true; }

    IRenderable* GetRenderable() override;
    
    // エディタのRaycast用
    Sphere GetWorldSphere() const;
    bool Raycast(const Ray& ray, float& outDistance) const override;

    std::string GetComponentName() const override { return "ModelBatchRendererComponent"; }
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    /**
     * @brief バッチ描画するインスタンスを追加します。
     * @param t インスタンスのローカルトランスフォーム（またはワールド）
     */
    void AddInstance(const Transform& t);

    /**
     * @brief ワールド行列を直接指定してインスタンスを追加します。
     * @param world ワールド行列
     */
    void AddInstanceWorld(const Matrix4x4& world);

    /**
     * @brief 登録されたインスタンスをすべてクリアします。毎フレーム呼ぶ必要があります。
     */
    void ClearInstances();

#ifdef EditorMode
    friend class ModelBatchRendererComponentEditor;
#endif

private:
    std::unique_ptr<ModelBatch> batch_;           ///< 実際のバッチ描画を担うクラス
    TransformComponent* transform_ = nullptr;     ///< 親のTransform情報（キャッシュ）
    std::string modelName_ = "plane.obj";          ///< 読み込むモデル名
};
