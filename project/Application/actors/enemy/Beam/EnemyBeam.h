#pragma once
#include "Irufemi.h"
#include "core/math/Transform.h"
#include "core/math/geometry/OBB.h"
#include <memory>
#include <wrl.h>
#include "IrufemiEngine/Renderer/ParticleGPU/GPUParticleSystem.h"
#include "IrufemiEngine/Engine/Graphics/Data/LightningParams.h"
#include "IrufemiEngine/Renderer/Object3D/Primitive/CylinderClass.h"
#include "IrufemiEngine/Renderer/Object3D/Primitive/PrimitiveObjects3DClass.h"
class Camera;

class EnemyBeam {
public:
    /**
     * @brief 初期化（リソース確保とプリロード）
     * @param camera カメラ
     * @param engine エンジンポインタ
     */
    ~EnemyBeam();
    void Initialize(class IrufemiEngine* engine);

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
    /** @brief 予兆ビームの太さ（直径）設定 */
    void SetTelegraphThickness(float thickness) { telegraphThickness_ = thickness; }
    /** @brief 予兆ビームの色設定 */
    void SetTelegraphColor(const Vector4& color) { if (telegraphObj_) telegraphObj_->SetColor(color); }

    /** @brief 攻撃ビームの表示設定 */
    void SetAttackActive(bool active) { 
        isAttackActive_ = active; 
        if (!active && gpuParticle_) {
            gpuParticle_->SetEmit(false);
        }
    }
    /** @brief 攻撃ビームがアクティブか */
    bool IsAttackActive() const { return isAttackActive_; }
    /** @brief 攻撃ビームの太さ（直径）設定 */
    void SetAttackThickness(float thickness) { attackThickness_ = thickness; }
    /** @brief 攻撃ビームの色設定 */
    void SetAttackColor(const Vector4& color) { if (attackCylinder_) attackCylinder_->SetColor(color); }

    /** @brief 攻撃判定用のOBBを取得 */
    OBB GetOBB() const;
    /** @brief 消滅フラグの取得 */
    bool IsExpired() const { return isExpired_; }

    /**
     * @brief ビームの発生位置を前方にずらすオフセット量を設定する
     * @param offset 前方へのオフセット距離（頭の半径など）
     */
    void SetOriginOffset(float offset) { originOffset_ = offset; }

    /** @brief チャージ球の表示設定 */
    void SetChargeSphereActive(bool active) { isChargeSphereActive_ = active; }
    /** @brief チャージ球のスケール設定 */
    void SetChargeSphereScale(float scale) { chargeSphereScale_ = scale; }

private:
    float originOffset_ = 4.0f; //!< ビームの発生起点を前方にずらす距離

    // チャージ球体用
    std::unique_ptr<PrimitiveObjects3DClass> chargeSphere_ = nullptr;
    float chargeSphereScale_ = 0.0f;
    bool isChargeSphereActive_ = false;

    // 予兆用
    std::unique_ptr<PrimitiveObjects3DClass> telegraphObj_ = nullptr;
    Transform telegraphTransform_;
    float telegraphThickness_ = 0.2f;
    float telegraphForwardOffset_ = 0.0f;
    bool isTelegraphActive_ = false;

    // 攻撃用 (内側のレーザーコア)
    std::shared_ptr<CylinderClass> attackCylinder_ = nullptr;
    // 攻撃用 (外側の電撃オーラ)
    std::shared_ptr<CylinderClass> attackCylinderOuter_ = nullptr;
    Transform attackTransform_;
    float attackThickness_ = 0.5f;
    float attackForwardOffset_ = 0.0f;
    bool isAttackActive_ = false;

    // アニメーション用
    float attackTimer_ = 0.0f;
    float attackExpandTime_ = 0.15f; // ビームが最大まで伸びる時間(秒)

    // 電撃エフェクト用（内側レーザー用パラメータ）
    Microsoft::WRL::ComPtr<ID3D12Resource> lightningParamsResource_;
    LightningParams* lightningParamsData_ = nullptr;

    // 電撃エフェクト用（外側オーラ用パラメータ）
    Microsoft::WRL::ComPtr<ID3D12Resource> lightningParamsOuterResource_;
    LightningParams* lightningParamsOuterData_ = nullptr;

    // 放出エフェクト用（パーティクル表現）
    std::unique_ptr<GPUParticleSystem> gpuParticle_ = nullptr;

    float beamLength_ = 500.0f;
    bool isExpired_ = false;
    class IrufemiEngine* engine_ = nullptr;
};