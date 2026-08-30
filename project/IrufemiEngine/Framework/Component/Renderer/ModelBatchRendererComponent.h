#pragma once
#include "Core/Math/Math.h"
#include "Core/Math/Transform.h"
#include "Core/Shape/Sphere.h"
#include "Framework/Component/Component.h"
#include <memory>
#include <string>

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

    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override;
    /**
     * @brief Update を実行する。
     */
    void Update() override;
    /**
     * @brief Draw を実行する。
     */
    void Draw() override;

    /**
     * @brief CanUpdateInEditMode かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool CanUpdateInEditMode() const override {
        return true;
    }

    /**
     * @brief Renderable を取得する。
     * @return 取得された Renderable
     */
    IRenderable* GetRenderable() override;

    // エディタのRaycast用
    /**
     * @brief WorldSphere を取得する。
     * @return 取得された WorldSphere
     */
    Irufemi::Sphere GetWorldSphere() const;
    /**
     * @brief Raycast を実行する。
     */
    bool Raycast(const Irufemi::Ray& ray, float& outDistance) const override;

    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override {
        return "ModelBatchRendererComponent";
    }
    /**
     * @brief Serialize を実行する。
     */
    nlohmann::json Serialize() override;
    /**
     * @brief Deserialize を実行する。
     */
    void Deserialize(const nlohmann::json& j) override;

    /**
     * @brief 現在読み込まれているモデル名を取得します。
     * @return モデル名
     */
    const std::string& GetModelName() const {
        return modelName_;
    }

    /**
     * @brief バッチ描画するインスタンスを追加します。
     * @param t インスタンスのローカルトランスフォーム（またはワールド）
     */
    void AddInstance(const Irufemi::Transform& t, int32_t effectType = 0, float effectParam = 0.0f,
                     bool enableMask = false);

    /**
     * @brief ワールド行列を直接指定してインスタンスを追加します。
     * @param world ワールド行列
     */
    void AddInstanceWorld(const Irufemi::Matrix4x4& world, int32_t effectType = 0, float effectParam = 0.0f,
                          bool enableMask = false);

    /**
     * @brief 登録されたインスタンスをすべてクリアします。毎フレーム呼ぶ必要があります。
     */
    void ClearInstances();

    /**
     * @brief GPUフラスタムカリングを有効にするか設定します
     */
    void SetUseGPUCulling(bool use);

#ifdef EditorMode
    friend class ModelBatchRendererComponentEditor;
#endif

private:
    std::unique_ptr<ModelBatch> batch_;   ///< 実際のバッチ描画を担うクラス
    std::string modelName_ = "plane.obj"; ///< 読み込むモデル名
    bool useGPUCulling_ = false;          ///< GPUカリングの有効フラグ
};
