#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Transform.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"
#include "Engine/Graphics/Data/LightningParams.h"
#include <memory>
#include <wrl.h>

/**
 * @class EnemyBeamComponent
 * @brief 敵が発射するビーム演出を管理するコンポーネント
 */
class EnemyBeamComponent : public Component {
public:
    EnemyBeamComponent() = default;
    ~EnemyBeamComponent() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void OnRegisterProperties() override;
    
    std::string GetComponentName() const override { return "EnemyBeamComponent"; }

    /**
     * @brief ビーム発射シーケンスを開始する
     * @param startPos 発射元の座標
     * @param targetPos ターゲットの座標
     */
    void Fire(const Irufemi::Vector3& startPos, const Irufemi::Vector3& targetPos);

    bool IsActive() const { return state_ != State::IDLE; }

private:
    enum class State {
        IDLE,
        CHARGING,
        FIRING
    };

    State state_ = State::IDLE;

    // パラメータ
    Irufemi::Vector3 startPos_;
    Irufemi::Vector3 direction_;
    float beamLength_ = 200.0f;     // ビームの最大長
    float beamMaxRadius_ = 1.0f;    // カメラに収まる程度の細さに調整（元は太すぎた）
    float chargeDuration_ = 1.5f;   // 溜め時間
    float fireDuration_ = 0.8f;     // 発射時間

    Irufemi::Vector4 chargeColor_ = { 0.7f, 0.0f, 0.9f, 1.0f };

    Irufemi::Vector4 beamColor_ = { 0.8f, 0.0f, 1.0f, 1.0f };
    Irufemi::Vector4 beamCoreColor_ = { 0.0f, 1.0f, 1.0f, 1.0f };
    float beamIntensity_ = 6.0f;
    float beamCoreIntensity_ = 40.0f;
    float beamSpeed_ = 3.0f;

    Irufemi::Vector4 auraColor_ = { 0.1f, 0.0f, 0.2f, 1.0f };
    Irufemi::Vector4 auraCoreColor_ = { 0.8f, 0.0f, 1.0f, 1.0f };
    float auraIntensity_ = 12.0f;
    float auraSpeed_ = 0.8f;

    float stateTimer_ = 0.0f;

    // 描画オブジェクト
    std::unique_ptr<Primitive3DObject> chargeSphere_ = nullptr;
    std::shared_ptr<Primitive3DObject> attackCylinder_ = nullptr;      // 内側の極太レーザーコア
    std::shared_ptr<Primitive3DObject> attackCylinderOuter_ = nullptr; // 外側の電撃オーラ

    // シェーダーパラメータ
    Microsoft::WRL::ComPtr<ID3D12Resource> beamParamsResource_;
    LightningParams* beamParamsData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> auraParamsResource_;
    LightningParams* auraParamsData_ = nullptr;
};
