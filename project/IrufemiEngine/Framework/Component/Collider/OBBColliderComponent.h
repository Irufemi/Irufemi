#pragma once
#include "ColliderComponent.h"
#include "Engine/Core/Math/Geometry/OBB.h"
#include "Engine/Core/Math/Vector3.h"
#include <string>

class TransformComponent;

/**
 * @class OBBColliderComponent
 * @brief OBB (有向境界箱) を用いた衝突判定を行うコンポーネント
 * @details GameObjectにアタッチし、回転を伴う箱型の当たり判定領域を提供します。
 */
class OBBColliderComponent : public ColliderComponent {
public:
    /**
     * @brief コンストラクタ
     */
    OBBColliderComponent();

    /**
     * @brief デストラクタ
     */
    ~OBBColliderComponent() override;

    /**
     * @brief コンポーネントの初期化
     * @details アタッチされているGameObjectからTransformComponentを取得しキャッシュします。
     */
    void Initialize() override;

    /**
     * @brief 毎フレームの更新処理
     * @details ローカル情報とTransformを合成してワールド空間上のOBBを更新し、衝突マネージャに登録します。
     */
    void Update() override;

    /**
     * @brief デバッグ描画
     * @details 衝突領域をワイヤーフレームなどで可視化します。
     */
    void DrawDebug() override;

    /**
     * @brief シリアライズ
     * @return OBBのサイズやオフセット情報を含むJSONオブジェクト
     */
    nlohmann::json Serialize() override;

    /**
     * @brief デシリアライズ
     * @param[in] j 読み込むJSONオブジェクト
     */
    void Deserialize(const nlohmann::json& j) override;

    /**
     * @brief コンポーネント名の取得
     * @return "OBBColliderComponent"
     */
    std::string GetComponentName() const override { return "OBBColliderComponent"; }

    /**
     * @brief コライダーの型を取得
     * @return ColliderType::OBB
     */
    ColliderType GetColliderType() const override { return ColliderType::OBB; }

    /**
     * @brief 衝突判定用のブロードフェーズバウンディングボックスを取得
     * @return ワールド空間のAABB
     */
    Irufemi::AABB GetBoundingBox() const override;

    /**
     * @brief ワールド空間でのOBB情報を取得する
     * @return 計算済みのワールドOBB
     */
    Irufemi::OBB GetWorldOBB() const;

    /**
     * @brief ローカル空間でのオフセット座標を設定する
     * @param[in] offset ローカル座標系の中心からのズレ
     */
    void SetLocalOffset(const Irufemi::Vector3& offset) { localOffset_ = offset; }

    /**
     * @brief ローカル空間でのオフセット座標を取得する
     * @return ローカル座標系の中心からのズレ
     */
    const Irufemi::Vector3& GetLocalOffset() const { return localOffset_; }

    /**
     * @brief ローカル空間でのOBBのサイズ（各軸のExtents）を設定する
     * @param[in] size 幅・高さ・奥行きの半分（Extents）
     */
    void SetLocalSize(const Irufemi::Vector3& size) { localSize_ = size; }

    /**
     * @brief ローカル空間でのOBBのサイズを取得する
     * @return OBBのExtents
     */
    const Irufemi::Vector3& GetLocalSize() const { return localSize_; }

private:
    TransformComponent* transform_ = nullptr;

    Irufemi::Vector3 localOffset_ = { 0.0f, 0.0f, 0.0f };
    Irufemi::Vector3 localSize_   = { 1.0f, 1.0f, 1.0f }; // Extents
};
