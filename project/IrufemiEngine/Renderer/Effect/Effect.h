#include "../Core/IRenderable.h"
#pragma once

#include <memory>
#include <string>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Type/PrimitiveType.h"

class GPUParticleSystem;
class Camera;

/**
 * @enum EffectType
 * @brief エフェクトの種類を管理する列挙型
 */
enum class EffectType {
    kHit,       // ヒットエフェクト（星型に広がる斬撃など）
    // 今後増えるエフェクトの種類をここに追加
};

/**
 * @class Effect
 * @brief 汎用エフェクトクラス
 * @details EffectType を指定することで、適切な初期化・再生を行う
 */
class Effect : public IRenderable {
public:
    Effect();
    ~Effect();

    /**
     * @brief エフェクトの初期化
     * @param camera 使用するカメラ
     * @param type エフェクトの種類
     */
    void Initialize(Camera* camera, EffectType type);

    /**
     * @brief エフェクトの更新
     */
    void Update();

    /**
     * @brief エフェクトの描画
     */
    void SyncBeforeDraw() override;
    void Draw() override;

    /**
     * @brief デバッグUIの表示
     * @param name ImGui上で表示するノード名
     */
    void Debug(const char* name = "Effect");
    
    /**
     * @brief 指定した座標にエフェクトを発生させる
     * @param position 発生させるワールド座標
     */
    void Play(const Vector3& position);

struct HitEffectConfig {
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    float lifeMin = 1.0f;
    float lifeMax = 1.0f;
    float jitter = 0.0f;
    Vector3 startScaleMin = { 0.05f, 0.4f, 1.0f };
    Vector3 startScaleMax = { 0.05f, 1.5f, 1.0f };
    Vector3 endScaleMin = { 0.05f, 0.0f, 1.0f };
    Vector3 endScaleMax = { 0.05f, 0.0f, 1.0f };
    int emitCount = 8;
};

private:
    Camera* camera_ = nullptr; // 再初期化用にカメラを保持
    std::unique_ptr<GPUParticleSystem> gpuParticleSystem_;
    EffectType type_;

    HitEffectConfig hitConfig_;
    
    // 全エフェクト共通の設定
    PrimitiveType currentShape_ = PrimitiveType::Plane;
    std::string currentTextureName_ = "resources/circle2.png";
};


