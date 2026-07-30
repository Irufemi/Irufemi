#pragma once
#include "../Component.h"
#include <string>
#include <memory>
#include "Renderer/Object/Particle/ParticleObject.h"

class TransformComponent;

/**
 * @class ParticleEmitterComponent
 * @brief エディタ用コンポーネント。実際の描画・ロジックは内部の ParticleObject に委譲します。
 */
class ParticleEmitterComponent : public Component {
public:
    ParticleEmitterComponent();
    ~ParticleEmitterComponent() override;

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
     * @brief Renderable を取得する。
     * @return 取得された Renderable
     */
    IRenderable* GetRenderable() override { return nullptr; }

    /**
     * @brief CanUpdateInEditMode かどうかを判定する。
     * @return 判定結果 (true/false)
     */
    bool CanUpdateInEditMode() const override { return true; } // エディタでのプレビュー更新を許可

    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "ParticleEmitterComponent"; }
    /**
     * @brief OnRegisterProperties を実行する。
     */
    void OnRegisterProperties() override;

    /**
     * @brief Serialize を実行する。
     */
    nlohmann::json Serialize() override;
    /**
     * @brief Deserialize を実行する。
     */
    void Deserialize(const nlohmann::json& j) override;

    /**
     * @brief Play を実行する。
     */
    void Play();
    /**
     * @brief Restart を実行する。
     */
    void Restart(bool withChildren = true);
    /**
     * @brief Stop を実行する。
     */
    void Stop();
    
    /**
     * @brief EmitBurst を実行する。
     */
    void EmitBurst(int count);

    // エディタや他スクリプトから実体へアクセスするためのゲッター
    /**
     * @brief ParticleObject を取得する。
     * @return 取得された ParticleObject
     */
    ParticleObject* GetParticleObject() const { return particleObj_.get(); }

private:
    std::unique_ptr<ParticleObject> particleObj_;
    TransformComponent* transform_ = nullptr;
};
