#include "../Core/IRenderable.h"
#pragma once

#include <memory>
#include <string>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Type/PrimitiveType.h"
#include "Engine/Core/Type/BlendMode.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include <vector>
#include <memory>

class GPUParticleSystem;
class PrimitiveObjects3DClass;

/**
 * @enum EffectType
 * @brief エフェクトの種類を管理する列挙型
 */
enum class EffectType {
    kHit,       // ヒットエフェクト（星型に広がる斬撃など）
    kImpact, // スライドの表現（PlaneとRingの複合ヒットエフェクト）
    kAura,      // オーラエフェクト
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
    void Initialize(EffectType type);

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
    static void SetEngine(class IrufemiEngine* engine) { engine_ = engine; }
    
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

struct ImpactConfig {
    PrimitiveType planeShape = PrimitiveType::Plane;
    std::string planeTexture = "resources/circle2.png";
    PrimitiveType ringShape = PrimitiveType::Ring;
    std::string ringTexture = "resources/gradationLine.png";

    Vector2 uvScale = { 5.0f, 1.0f }; // RingのU方向スケール
    Vector2 uvScrollSpeed = { 1.0f, 0.0f }; // Ringのスクロール速度
    bool useClamp = true; // Ringの白丸回避用
    
    // Plane固有設定
    bool planeEnableRandomRotation = true;
    int planeEmitCount = 4;
    
    // Ring固有設定
    bool ringEnableRandomRotation = false;
    int ringEmitCount = 1;
    
    float jitter = 0.0f; // 座標のゆらぎ（時間経過での移動を防ぐため0.0）
    Vector3 planeStartScaleMin = { 0.05f, 0.4f, 1.0f };
    Vector3 planeStartScaleMax = { 0.05f, 1.5f, 1.0f };
    Vector3 ringStartScaleMin = { 0.8f, 0.8f, 1.0f };
    Vector3 ringStartScaleMax = { 0.8f, 0.8f, 1.0f };
    Vector3 planeEndScaleMin = { 0.05f, 0.0f, 1.0f };
    Vector3 planeEndScaleMax = { 0.05f, 0.0f, 1.0f };
    Vector3 ringEndScaleMin = { 0.0f, 0.0f, 0.0f };
    Vector3 ringEndScaleMax = { 0.0f, 0.0f, 0.0f };
    float lifeMin = 2.0f;
    float lifeMax = 2.0f;
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct AuraConfig {
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector2 uvScrollSpeed = { -0.1f, 0.0f }; // 横にゆっくり流れるように変更
    bool flipV = true;
    bool useClamp = true; // 新規追加したサンプラー(U:Wrap, V:Clamp)が適用されるためtrueをデフォルトに
    std::string texture = "resources/gradationLine.png";
};

private:
    static class IrufemiEngine* engine_;
    std::vector<std::unique_ptr<GPUParticleSystem>> particleSystems_;
    std::unique_ptr<PrimitiveObjects3DClass> auraObject_;
    EffectType type_;

    HitEffectConfig hitConfig_;
    ImpactConfig impactConfig_;
    AuraConfig auraConfig_;
    Vector2 currentUVOffset_ = { 0.0f, 0.0f };
    
    // 全エフェクト共通の設定
    PrimitiveType currentShape_ = PrimitiveType::Plane;
    std::string currentTextureName_ = "resources/circle2.png";

    // 描画設定
    BlendMode blendMode_ = BlendMode::kBlendModeAdd;
    PSOManager::DepthWrite depthWrite_ = PSOManager::DepthWrite::Disable;
    PSOManager::CullMode cullMode_ = PSOManager::CullMode::None;
    bool isBillboard_ = true;
};


