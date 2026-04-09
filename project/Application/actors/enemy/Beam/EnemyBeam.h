#pragma once
#include "Irufemi.h"
#include "core/math/Transform.h"
#include "core/math/geometry/OBB.h"
#include <memory>

class Camera;

class EnemyBeam {
public:
    /**
     * @brief 初期化
     * @param camera カメラ
     * @param muzzleMatrix 発射口のワールド行列
     */
    void Initialize(Camera* camera, const Matrix4x4& muzzleMatrix);

    /**
     * @brief 更新
     * @param headPos 頭部の位置（始点）
     * @param playerPos プレイヤーの位置（ターゲット点）
     */
    void Update(const Vector3& headPos, const Vector3& playerPos);

    /**
     * @brief 描画
     * @param engine エンジンポインタ
     */
    void Draw(class IrufemiEngine* engine);

    /** @brief 予兆ビームの表示設定 */
    void SetTelegraphActive(bool active) { isTelegraphActive_ = active; }
    /** @brief 予兆ビームがアクティブか */
    bool IsTelegraphActive() const { return isTelegraphActive_; }
    /** @brief 予兆ビームの太さ設定 */
    void SetTelegraphThickness(float thickness) { telegraphThickness_ = thickness; }
    /** @brief 予兆ビームの色設定 */
    void SetTelegraphColor(const Vector4& color) { if (telegraphObj_) telegraphObj_->SetColor(color); }

    /** @brief 攻撃ビームの表示設定 */
    void SetAttackActive(bool active) { isAttackActive_ = active; }
    /** @brief 攻撃ビームがアクティブか */
    bool IsAttackActive() const { return isAttackActive_; }
    /** @brief 攻撃ビームの太さ設定 */
    void SetAttackThickness(float thickness) { attackThickness_ = thickness; }
    /** @brief 攻撃ビームの色設定 */
    void SetAttackColor(const Vector4& color) { if (attackObj_) attackObj_->SetColor(color); }

    /** @brief 攻撃判定用のOBBを取得 */
    OBB GetOBB() const;
    /** @brief 消滅フラグの取得 */
    bool IsExpired() const { return isExpired_; }

private:
    // 予兆用
    std::unique_ptr<ObjClass> telegraphObj_ = nullptr;
    Transform telegraphTransform_;
    float telegraphThickness_ = 0.2f;
    float telegraphForwardOffset_ = 0.0f;
    bool isTelegraphActive_ = false;

    // 攻撃用
    std::unique_ptr<ObjClass> attackObj_ = nullptr;
    Transform attackTransform_;
    float attackThickness_ = 1.0f;
    float attackForwardOffset_ = 0.0f;
    bool isAttackActive_ = false;

    float beamLength_ = 100.0f;
    bool isExpired_ = false;
};