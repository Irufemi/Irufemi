#pragma once
#include <string>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "Renderer/System/ParticleGPU/GPUParticleManager.h"

#include "Engine/Core/Type/BlendMode.h"
#include <nlohmann/json.hpp>

class TextureManager;

/**
 * @class ParticleObject
 * @brief C++べた書きでGPUパーティクルを制御するための描画オブジェクトラッパー
 */
class ParticleObject {
public:
    ParticleObject();
    ~ParticleObject();

    void Initialize();
    void Play();
    void Stop();
    void EmitBurst(int count);
    void Update();

    static void SetGPUParticleManager(GPUParticleManager* manager) { gpuParticleManager_ = manager; }
    
private:
    inline static GPUParticleManager* gpuParticleManager_ = nullptr;
public:
    void DebugUI(const char* name = "Particle Object");

    void Serialize(nlohmann::json& j) const;
    void Deserialize(const nlohmann::json& j);
    bool LoadFromJson(const std::string& filepath);

    // --- Getters & Setters ---

    /**
     * @brief パーティクル発生の中心座標を設定し、変更があればGPU転送フラグを立てます
     * @param pos 新しいワールド座標
     */
    void SetPosition(const Vector3& pos) { if (position_.x != pos.x || position_.y != pos.y || position_.z != pos.z) { position_ = pos; MarkDirty(); } }
    Vector3 GetPosition() const { return position_; }

    void SetRotation(const Vector3& rot) { if (rotation_.x != rot.x || rotation_.y != rot.y || rotation_.z != rot.z) { rotation_ = rot; MarkDirty(); } }
    Vector3 GetRotation() const { return rotation_; }

    void SetTexturePath(const std::string& path);
    std::string GetTexturePath() const { return texturePath_; }

    void SetBlendMode(BlendMode mode);
    BlendMode GetBlendMode() const { return blendMode_; }

    void SetUnscaledTime(bool val);
    bool IsUnscaledTime() const { return isUnscaledTime_; }

    void SetEmitOnAwake(bool val) { emitOnAwake_ = val; }
    bool GetEmitOnAwake() const { return emitOnAwake_; }

    void SetEmitType(int type) { if (emitType_ != type) { emitType_ = type; MarkDirty(); } }
    int GetEmitType() const { return emitType_; }

    void SetEmissionRate(float rate) { if (emissionRate_ != rate) { emissionRate_ = rate; MarkDirty(); } }
    float GetEmissionRate() const { return emissionRate_; }

    void SetLifeTimeMin(float life) { if (lifeTimeMin_ != life) { lifeTimeMin_ = life; MarkDirty(); } }
    float GetLifeTimeMin() const { return lifeTimeMin_; }

    void SetLifeTimeMax(float life) { if (lifeTimeMax_ != life) { lifeTimeMax_ = life; MarkDirty(); } }
    float GetLifeTimeMax() const { return lifeTimeMax_; }

    void SetVelocity(float vel) { if (velocity_ != vel) { velocity_ = vel; MarkDirty(); } }
    float GetVelocity() const { return velocity_; }

    void SetRadius(float rad) { if (radius_ != rad) { radius_ = rad; MarkDirty(); } }
    float GetRadius() const { return radius_; }

    void SetSpread(float spr) { if (spread_ != spr) { spread_ = spr; MarkDirty(); } }
    float GetSpread() const { return spread_; }

    void SetAtlasRows(int rows) { if (atlasRows_ != rows) { atlasRows_ = rows; MarkDirty(); } }
    int GetAtlasRows() const { return atlasRows_; }

    void SetAtlasCols(int cols) { if (atlasCols_ != cols) { atlasCols_ = cols; MarkDirty(); } }
    int GetAtlasCols() const { return atlasCols_; }

    void SetGravity(float grav) { if (gravity_ != grav) { gravity_ = grav; MarkDirty(); } }
    float GetGravity() const { return gravity_; }

    void SetDamping(float damp) { if (damping_ != damp) { damping_ = damp; MarkDirty(); } }
    float GetDamping() const { return damping_; }

    void SetBounce(float bounce) { if (bounce_ != bounce) { bounce_ = bounce; MarkDirty(); } }
    float GetBounce() const { return bounce_; }

    void SetGroundHeight(float height) { if (groundHeight_ != height) { groundHeight_ = height; MarkDirty(); } }
    float GetGroundHeight() const { return groundHeight_; }

    void SetAttractorStrength(float strength) { if (attractorStrength_ != strength) { attractorStrength_ = strength; MarkDirty(); } }
    float GetAttractorStrength() const { return attractorStrength_; }

    void SetAttractorPos(const Vector3& pos) { if (attractorPos_.x != pos.x || attractorPos_.y != pos.y || attractorPos_.z != pos.z) { attractorPos_ = pos; MarkDirty(); } }
    Vector3 GetAttractorPos() const { return attractorPos_; }

    void SetJitter(float jitter) { if (jitter_ != jitter) { jitter_ = jitter; MarkDirty(); } }
    float GetJitter() const { return jitter_; }

    void SetBillboardMode(int mode) { if (billboardMode_ != mode) { billboardMode_ = mode; MarkDirty(); } }
    int GetBillboardMode() const { return billboardMode_; }

    void SetColor(const Vector4& color) { if (color_.x != color.x || color_.y != color.y || color_.z != color.z || color_.w != color.w) { color_ = color; MarkDirty(); } }
    Vector4 GetColor() const { return color_; }

    void SetMidColor(const Vector4& color) { if (midColor_.x != color.x || midColor_.y != color.y || midColor_.z != color.z || midColor_.w != color.w) { midColor_ = color; MarkDirty(); } }
    Vector4 GetMidColor() const { return midColor_; }

    void SetStartScale(const Vector3& scale) { if (startScale_.x != scale.x || startScale_.y != scale.y || startScale_.z != scale.z) { startScale_ = scale; MarkDirty(); } }
    Vector3 GetStartScale() const { return startScale_; }

    void SetMidScale(const Vector3& scale) { if (midScale_.x != scale.x || midScale_.y != scale.y || midScale_.z != scale.z) { midScale_ = scale; MarkDirty(); } }
    Vector3 GetMidScale() const { return midScale_; }

    void SetEndScale(const Vector3& scale) { if (endScale_.x != scale.x || endScale_.y != scale.y || endScale_.z != scale.z) { endScale_ = scale; MarkDirty(); } }
    Vector3 GetEndScale() const { return endScale_; }

    void SetMidPoint(float point) { if (midPoint_ != point) { midPoint_ = point; MarkDirty(); } }
    float GetMidPoint() const { return midPoint_; }

    void SetDirection(const Vector3& dir) { if (direction_.x != dir.x || direction_.y != dir.y || direction_.z != dir.z) { direction_ = dir; MarkDirty(); } }
    Vector3 GetDirection() const { return direction_; }

    void SetAreaSize(const Vector3& size) { if (areaSize_.x != size.x || areaSize_.y != size.y || areaSize_.z != size.z) { areaSize_ = size; MarkDirty(); } }
    Vector3 GetAreaSize() const { return areaSize_; }

    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static TextureManager* GetTextureManager() { return textureManager_; }

    void MarkDirty() { isDirty_ = true; }

private:
    // トランスフォーム
    Vector3 position_ = { 0.0f, 0.0f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f }; // 現在は主に方向として使用

    std::string texturePath_ = "resources/circle.png";
    BlendMode blendMode_ = BlendMode::kBlendModeAdd;
    bool isUnscaledTime_ = false;
    bool emitOnAwake_ = true;
    
    // エミッターの基本パラメータ
    int emitType_ = 0; // 0: Sphere, 1: Beam, 2: Box, 3: Cylinder
    float emissionRate_ = 50.0f; // 1秒あたりの発生数
    float lifeTimeMin_ = 0.5f;
    float lifeTimeMax_ = 1.0f;
    float velocity_ = 1.0f;
    float radius_ = 1.0f;
    float spread_ = 0.1f;
    
    // アニメーション設定
    int atlasRows_ = 1;
    int atlasCols_ = 1;

    // 物理・挙動パラメータ
    float gravity_ = 0.0f;
    float damping_ = 0.0f;
    float bounce_ = 0.0f;          // 床でのバウンド係数
    float groundHeight_ = -100.0f; // 床のY座標
    float attractorStrength_ = 0.0f; // 吸引力
    Vector3 attractorPos_ = {0.0f, 0.0f, 0.0f}; // 吸引位置
    float jitter_ = 0.0f;          // 不規則なブレ

    // ビジュアル・ライフタイム
    int billboardMode_ = 1; // 0: None, 1: Billboard, 2: Y-Axis
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector4 midColor_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // 中間色
    Vector3 startScale_ = { 1.0f, 1.0f, 1.0f };
    Vector3 midScale_ = { 1.0f, 1.0f, 1.0f };       // 中間スケール
    Vector3 endScale_ = { 0.0f, 0.0f, 0.0f };
    float midPoint_ = 0.5f;                         // 中間点の位置(0.0~1.0)
    
    Vector3 direction_ = { 0.0f, 0.0f, 1.0f };
    Vector3 areaSize_ = { 10.0f, 10.0f, 10.0f };    // Boxエミッター用サイズ

private:
    void UpdateSystem();

    static TextureManager* textureManager_;
    GPUParticleManager::EmitterHandle emitterHandle_;
    bool isPlaying_ = false;
    int burstCountPending_ = 0;
    bool isDirty_ = true; // パラメータ変更検知フラグ
};
