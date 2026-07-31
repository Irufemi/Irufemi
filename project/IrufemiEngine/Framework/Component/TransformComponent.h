#pragma once
#include "Component.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Core/System/ComponentPool.h"

class TransformComponent : public Component {
public:
    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override {}
    // Update() は一括更新 (UpdateAll) に移行したため完全に削除し、GameObjectのUpdateループから外す
    
    /**
     * @brief 自身の行列を計算する（Dirtyフラグベースの遅延評価付き）
     * @param force 親が変更された等の理由による強制更新フラグ
     */
    void ComputeMatrix(bool force = false);

    /**
     * @brief プール上の全Transformを一括更新する（DOD）
     */
    static void UpdateAll();

    /**
     * @brief CanUpdateInEditMode かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool CanUpdateInEditMode() const override { return true; }

    // --- Getters ---
    /**
     * @brief Position を取得する。
     * @return 取得された Position
     */
    const Irufemi::Vector3& GetPosition() const { return position_; }
    /**
     * @brief Rotation を取得する。
     * @return 取得された Rotation
     */
    const Irufemi::Vector3& GetRotation() const { return rotation_; }
    /**
     * @brief Scale を取得する。
     * @return 取得された Scale
     */
    const Irufemi::Vector3& GetScale() const { return scale_; }

    /**
     * @brief WorldMatrix を取得する。
     * @return 取得された WorldMatrix
     */
    const Irufemi::Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
    /**
     * @brief LocalMatrix を取得する。
     * @return 取得された LocalMatrix
     */
    const Irufemi::Matrix4x4& GetLocalMatrix() const { return localMatrix_; }

    // ワールド情報の遅延抽出 Getter
    /**
     * @brief WorldPosition を取得する。
     * @return 取得された WorldPosition
     */
    const Irufemi::Vector3& GetWorldPosition() const;
    /**
     * @brief WorldRotation を取得する。
     * @return 取得された WorldRotation
     */
    const Irufemi::Vector3& GetWorldRotation() const;
    /**
     * @brief WorldScale を取得する。
     * @return 取得された WorldScale
     */
    const Irufemi::Vector3& GetWorldScale() const;

    // ワールド方向ベクトルの抽出 (正規化済み)
    /**
     * @brief WorldRight を取得する。
     * @return 取得された WorldRight
     */
    Irufemi::Vector3 GetWorldRight() const;
    /**
     * @brief WorldUp を取得する。
     * @return 取得された WorldUp
     */
    Irufemi::Vector3 GetWorldUp() const;
    /**
     * @brief WorldForward を取得する。
     * @return 取得された WorldForward
     */
    Irufemi::Vector3 GetWorldForward() const;

    // --- Setters ---
    /**
     * @brief Position を設定する。
     * @param[in] position 設定する Position の値
     */
    void SetPosition(const Irufemi::Vector3& position);
    /**
     * @brief Rotation を設定する。
     * @param[in] rotation 設定する Rotation の値
     */
    void SetRotation(const Irufemi::Vector3& rotation);
    /**
     * @brief Scale を設定する。
     * @param[in] scale 設定する Scale の値
     */
    void SetScale(const Irufemi::Vector3& scale);

    /**
     * @brief ワールド行列の再計算を強制する（親が変更された時などに使用）
     */
    void MarkWorldDirty() { isWorldDirty_ = true; }

    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "TransformComponent"; }
    /**
     * @brief Serialize を実行する。
     */
    nlohmann::json Serialize() override;
    /**
     * @brief Deserialize を実行する。
     */
    void Deserialize(const nlohmann::json& j) override;

#ifdef EditorMode

#endif

private:
    // --- Local Irufemi::Transform Data ---
    Irufemi::Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Irufemi::Vector3 rotation_ = { 0.0f, 0.0f, 0.0f }; // Euler angles in radians
    Irufemi::Vector3 scale_ = { 1.0f, 1.0f, 1.0f };

    // --- World Irufemi::Transform Data (Lazy Evaluated) ---
    mutable Irufemi::Vector3 worldPosition_ = { 0.0f, 0.0f, 0.0f };
    mutable Irufemi::Vector3 worldRotation_ = { 0.0f, 0.0f, 0.0f };
    mutable Irufemi::Vector3 worldScale_ = { 1.0f, 1.0f, 1.0f };

    // --- Matrices ---
    Irufemi::Matrix4x4 localMatrix_ = Irufemi::Math::MakeIdentity4x4();
    Irufemi::Matrix4x4 worldMatrix_ = Irufemi::Math::MakeIdentity4x4();

    // --- Flags ---
    bool isLocalDirty_ = true;
    bool isWorldDirty_ = true;
    mutable bool isWorldTransformExtracted_ = false;

    // UpdateAll用のフレームキャッシュ
    uint32_t lastCheckedFrame_ = 0;
    uint32_t lastWorldMatrixUpdateFrame_ = 0;
    static inline uint32_t currentFrame_ = 1;
};

