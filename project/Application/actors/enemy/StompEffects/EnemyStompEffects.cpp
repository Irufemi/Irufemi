#include "EnemyStompEffects.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Renderer/Object3D/Primitive/PrimitiveObjects3DClass.h"
#include "IrufemiEngine/Engine/IrufemiEngine.h"
#include "Irufemi.h" 
#include "Core/Math/Math.h"
#include <cmath>
#include <algorithm>

void EnemyStompEffects::Initialize(IrufemiEngine* engine) {
    explosionObj_ = std::make_unique<PrimitiveObjects3DClass>();
    explosionObj_->Initialize(PrimitiveType::Sphere, "resources/whiteTexture.png");
    explosionObj_->SetColor({1.0f, 0.5f, 0.2f, 1.0f});

    ringObj_ = std::make_unique<PrimitiveObjects3DClass>();
    ringObj_->Initialize(PrimitiveType::Cylinder, "resources/whiteTexture.png");
    ringObj_->SetColor({1.0f, 0.2f, 0.0f, 0.5f});
    ringObj_->SetCastShadows(false);

    if (engine) {
        bodyTelegraphObj_ = std::make_unique<PrimitiveObjects3DClass>();
        bodyTelegraphObj_->Initialize(PrimitiveType::Plane, "resources/whiteTexture.png");
        bodyTelegraphObj_->GetMaterial().enableLighting = false; // ライティング無効
        bodyTelegraphObj_->SetCastShadows(false); // 影を落とさない
        // AOEWarningシェーダー（加算、半透明など）を適用、地形による見切れを防ぐため深度テストOff
        bodyTelegraphObj_->SetCustomPSO(engine->GetPSOManager()->GetPSO(
            "AOEWarning", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Off, PSOManager::CullMode::None));
        
        aoeParamsResource_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(AOEParams));
        aoeParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&aoeParamsData_));
        if (aoeParamsData_) {
            *aoeParamsData_ = AOEParams();
            aoeParamsData_->shapeType = 0; // Radial
        }
        bodyTelegraphObj_->SetCustomCBVAddress(aoeParamsResource_->GetGPUVirtualAddress());
    }

    // GPUParticleSystemの初期化
    gpuParticleSystem_ = std::make_unique<GPUParticleSystem>();
    gpuParticleSystem_->Initialize("resources/circle.png");
    gpuParticleSystem_->SetBlend(BlendMode::kBlendModeAdd);
    gpuParticleSystem_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    gpuParticleSystem_->SetCull(PSOManager::CullMode::None);
    gpuParticleSystem_->SetCustomPSO("StompExplosionParticle"); // 新規カスタムPSOを適用
    gpuParticleSystem_->SetEmit(false);
    
    // パーティクルの基本設定 (爆発らしくするための調整)
    gpuParticleSystem_->SetParticleLife(1.0f, 2.5f); // 少し長めに残して余韻を作る
    // サイズ: 小さく細かく発生させ、高密度感を出す
    gpuParticleSystem_->SetParticleScale({ 0.5f, 0.5f, 0.5f }, { 1.5f, 1.5f, 1.5f }, { 2.5f, 2.5f, 2.5f }, { 5.0f, 5.0f, 5.0f });
    // 物理挙動: 急激に減速し、熱気で上に舞い上がるようにする
    gpuParticleSystem_->SetDamping(0.06f);  // 高い空気抵抗（初速は早いがすぐ減速する）
    gpuParticleSystem_->SetGravity(-0.2f); // 煙としてゆっくり上に昇るようにマイナス重力を弱める
    
    // 色は炎っぽく設定
    gpuParticleSystem_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    gpuParticleSystem_->SetStartColor({ 1.0f, 0.8f, 0.2f, 1.0f }, { 1.0f, 0.5f, 0.0f, 1.0f });
    gpuParticleSystem_->SetEndColor({ 1.0f, 0.1f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.0f });

    isActive_ = false;
}

void EnemyStompEffects::Fire(const Vector3& position) {
    if (isActive_) return;

    isActive_ = true;
    currentPhase_ = Phase::Expanding;
    basePosition_ = position;
    globalTimer_ = 0.0f;
    phaseTimer_ = 0.0f;
    hasDealtExplosionDamage_ = false;
    hasDealtRingDamage_ = false;
    hasDealtFinalDamage_ = false;


    // スケール初期化（前回の攻撃の残りをクリア）
    explosionTransform_.scale = { 1.0f, 1.0f, 1.0f };
    ringTransform_.scale = { 1.0f, params_.ringHeight, 1.0f };
    
    // 座標の初期化（X, Y, Z すべてに basePosition_ を適用）
    Vector3 spawnPos = { basePosition_.x, basePosition_.y + params_.ringGroundOffset, basePosition_.z };
    explosionTransform_.translate = spawnPos;
    ringTransform_.translate = spawnPos;

    UpdateFinalExplosionSphere();
}

void EnemyStompEffects::Cancel() {
    isActive_ = false;
    isBodyTelegraphActive_ = false;
    currentPhase_ = Phase::Finished;
    
    // スケールを0にして確実に画面から消す
    explosionTransform_.scale = { 0.0f, 0.0f, 0.0f };
    ringTransform_.scale = { 0.0f, 0.0f, 0.0f };
    bodyTelegraphTransform_.scale = { 0.0f, 0.0f, 0.0f };

    if (gpuParticleSystem_) {
        gpuParticleSystem_->Clear();
    }
}

void EnemyStompEffects::Update(float deltaTime) {
    if (!isActive_) return;


    globalTimer_ += deltaTime;
    phaseTimer_ += deltaTime;

    // 球（explosionObj_）の現在のスケールを計算（全体を通して使用）
    float totalWarningTime = params_.ringExpandDuration + params_.ringKeepDuration;
    float globalT = (std::min)(1.0f, globalTimer_ / totalWarningTime);
    float easeIn = EaseInQuad(globalT);
    float currentSphereScale = Lerp(1.0f, params_.ringMaxRadius, easeIn);

    if (currentPhase_ == Phase::Expanding || currentPhase_ == Phase::KeepAndWarning) {
        explosionTransform_.scale = { currentSphereScale, currentSphereScale, currentSphereScale };
        explosionTransform_.translate = { basePosition_.x, basePosition_.y + params_.ringGroundOffset, basePosition_.z };
        
        Vector4 colorBase = { 1.0f, 0.4f, 0.0f, 0.2f }; 
        Vector4 colorWarning = { 1.0f, 0.0f, 0.0f, 0.6f };
        Vector4 currentColor;
        currentColor.x = Lerp(colorBase.x, colorWarning.x, globalT);
        currentColor.y = Lerp(colorBase.y, colorWarning.y, globalT);
        currentColor.z = Lerp(colorBase.z, colorWarning.z, globalT);
        currentColor.w = Lerp(colorBase.w, colorWarning.w, globalT);
        
        float pulse = (std::sin(globalTimer_ * 15.0f * globalT) * 0.5f + 0.5f);
        currentColor.w += pulse * 0.2f * globalT; 

        explosionObj_->SetColor(currentColor);
    }

    // 球の広がりに合わせたリングの色比率 (0.0 ~ 1.0)
    float colorRatio = 0.0f;
    if (params_.ringMaxRadius > 1.0f) {
        // 球のスケール値から現在どこまで広がっているかを逆算して適用する
        float rawRatio = (currentSphereScale - 1.0f) / (params_.ringMaxRadius - 1.0f);
        rawRatio = (std::min)(1.0f, (std::max)(0.0f, rawRatio));
        
        // 色が変わる（黄色になる）タイミングが早く感じないよう、
        // EaseInQuart(4乗カーブ)を使って、最後の最後に一気に黄色になりきるようにする
        colorRatio = EaseInQuart(rawRatio);
    }
    colorRatio = (std::min)(1.0f, (std::max)(0.0f, colorRatio));

    Vector4 ringCurrentColor;
    ringCurrentColor.x = Lerp(params_.ringColorNormal.x, params_.ringColorWarning.x, colorRatio);
    ringCurrentColor.y = Lerp(params_.ringColorNormal.y, params_.ringColorWarning.y, colorRatio);
    ringCurrentColor.z = Lerp(params_.ringColorNormal.z, params_.ringColorWarning.z, colorRatio);
    ringCurrentColor.w = Lerp(params_.ringColorNormal.w, params_.ringColorWarning.w, colorRatio);

    switch (currentPhase_) {
    case Phase::Expanding:
    {
        float t = (std::min)(1.0f, phaseTimer_ / params_.ringExpandDuration);
        float currentRadius = Lerp(1.0f, params_.ringMaxRadius, t);

        ringTransform_.scale = { currentRadius, params_.ringHeight, currentRadius };
        ringObj_->SetColor(ringCurrentColor);

        if (t >= 1.0f) {
            currentPhase_ = Phase::KeepAndWarning;
            phaseTimer_ = 0.0f;
        }
    }
    break;

    case Phase::KeepAndWarning:
    {
        float t = (std::min)(1.0f, phaseTimer_ / params_.ringKeepDuration);

        ringTransform_.scale = { params_.ringMaxRadius, params_.ringHeight, params_.ringMaxRadius };
        ringObj_->SetColor(ringCurrentColor);

        if (t >= 1.0f) {
            currentPhase_ = Phase::FinalExplosion;
            phaseTimer_ = 0.0f;
            hasDealtFinalDamage_ = false;
            
            if (gpuParticleSystem_) {
                float burstRadius = 15.0f; // より広範囲から発生させる
                gpuParticleSystem_->SetHemisphereEmitter(
                    Vector3{ basePosition_.x, basePosition_.y + params_.ringGroundOffset + 1.0f, basePosition_.z },
                    burstRadius,
                    0, 
                    1.0f 
                );
                // Dampingが強いので、初速はドカンと速くする (フレーム単位の移動量なので5.0f程度)
                gpuParticleSystem_->SetVelocity(5.0f); 
                // XZ平面（横方向）への押し出しを強くする
                gpuParticleSystem_->SetSpread(3.0f); 
                // 密度を上げるため、一気に1万発出す（Max 32768 まで許容）
                gpuParticleSystem_->Emit(10000);
            }
        }
    }
    break;

    case Phase::FinalExplosion:
    {
        float t = (std::min)(1.0f, phaseTimer_ / params_.finalExplosionDuration);

        float easeOutR = EaseOutCubic(t);
        float currentR = Lerp(params_.ringMaxRadius, params_.finalExplosionMaxRadius, easeOutR);

        finalExplosionSphere_.center = Vector3{ basePosition_.x, basePosition_.y + params_.ringGroundOffset, basePosition_.z };
        finalExplosionSphere_.radius = currentR * 0.5f;

        if (t >= 1.0f) {
            currentPhase_ = Phase::Finished;
        }
    }
    break;

    case Phase::Finished:
    {
        // パーティクルの寿命(最大2.5秒)が終わるまで描画を待機し、完全に消滅したら非アクティブ化する
        if (phaseTimer_ > params_.finalExplosionDuration + 3.0f) {
            isActive_ = false;
        }
    }
    break;
    }

    if (explosionObj_) {
        explosionObj_->SetTransform(explosionTransform_);
        explosionObj_->Update();
    }
    if (ringObj_) {
        ringObj_->SetTransform(ringTransform_);
        ringObj_->Update();
    }

    if (gpuParticleSystem_) {
        gpuParticleSystem_->Update();
    }
}

void EnemyStompEffects::UpdateFinalExplosionSphere() {
    finalExplosionSphere_.center = Vector3{ basePosition_.x, basePosition_.y + params_.ringGroundOffset, basePosition_.z };
    finalExplosionSphere_.radius = params_.ringMaxRadius * 0.5f;
}

bool EnemyStompEffects::IsExplosionDamageActive() const {
    if (globalTimer_ >= params_.explosionDuration) return false;
    float t = globalTimer_ / params_.explosionDuration;
    return t < params_.explosionDamageActiveTime;
}

float EnemyStompEffects::GetExplosionRadius() const {
    float t = (std::min)(1.0f, globalTimer_ / params_.explosionDuration);
    float easeOut = EaseOutQuad(t);
    // params_.explosionMaxRadius は見た目の「直径」として設定されているため、当たり判定の「半径」にするために 0.5f を掛ける
    return Lerp(1.0f, params_.explosionMaxRadius * 0.5f, easeOut);
}

void EnemyStompEffects::StartBodyTelegraph(const Vector3& pos, float radius) {
    isBodyTelegraphActive_ = true;
    bodyTelegraphTransform_.scale = { radius, radius, 1.0f }; // PlaneはXY平面なのでXとYをスケーリング
    bodyTelegraphTransform_.rotate = { std::numbers::pi_v<float> / 2.0f, 0.0f, 0.0f }; // X軸で90度寝かせる
    bodyTelegraphTransform_.translate = pos; // 呼び出し元で指定された高さ（Y）をそのまま使う
    
    // Shader用パラメータ (Radial)
    if (aoeParamsData_) {
        aoeParamsData_->warningRatio = 0.0f;
    }
    
    // 赤色（少しオレンジを混ぜておく）、アルファはシェーダー内で調整される
    bodyTelegraphObj_->SetColor({ 1.0f, 0.1f, 0.0f, 0.8f });
    bodyTelegraphObj_->SetTransform(bodyTelegraphTransform_);
    bodyTelegraphObj_->Update();
}

void EnemyStompEffects::UpdateBodyTelegraph(const Vector3& pos, float warningRatio) {
    if (!isBodyTelegraphActive_ || !bodyTelegraphObj_) return;
    
    bodyTelegraphTransform_.translate = pos;
    bodyTelegraphObj_->SetTransform(bodyTelegraphTransform_);
    
    // Shader用パラメータ (Radial)
    if (aoeParamsData_) {
        aoeParamsData_->warningRatio = warningRatio;
    };
    
    bodyTelegraphObj_->Update();
}

void EnemyStompEffects::StopBodyTelegraph() {
    isBodyTelegraphActive_ = false;
    bodyTelegraphTransform_.scale = { 0.0f, 0.0f, 0.0f };
}

void EnemyStompEffects::DrawBodyTelegraph(IrufemiEngine* engine) {
    if (isBodyTelegraphActive_ && bodyTelegraphObj_ && engine) {
        bodyTelegraphObj_->Draw();
    }
}

void EnemyStompEffects::DrawDebug(Line3DRegion* lineRegion) {
    if (!lineRegion) return;

    if (isBodyTelegraphActive_) {
        // デバッグ用に黄色の線を引いて位置を確認
        Vector3 pos = bodyTelegraphTransform_.translate;
        Vector3 p1 = { pos.x - 3.0f, pos.y + 0.1f, pos.z - 3.0f };
        Vector3 p2 = { pos.x + 3.0f, pos.y + 0.1f, pos.z + 3.0f };
        lineRegion->AddInstance(p1, p2, { 1.0f, 1.0f, 0.0f, 1.0f });
    }

    if (!isActive_) return;

    if (globalTimer_ < params_.explosionDuration) {
        float t = globalTimer_ / params_.explosionDuration;
        float easeOut = EaseOutQuad(t);
        float currentRadius = Lerp(1.0f, params_.explosionMaxRadius, easeOut);
        Vector4 color = (t < params_.explosionDamageActiveTime) ? Vector4{ 1.0f, 0.0f, 0.0f, 1.0f } : Vector4{ 1.0f, 0.5f, 0.0f, 1.0f };

        const int segments = 16;
        const float step = (2.0f * 3.14159265f) / segments;

        for (int i = 0; i < segments; ++i) {
            float theta1 = i * step;
            float theta2 = (i + 1) * step;
            Vector3 p1 = { basePosition_.x + currentRadius * std::cos(theta1), basePosition_.y, basePosition_.z + currentRadius * std::sin(theta1) };
            Vector3 p2 = { basePosition_.x + currentRadius * std::cos(theta2), basePosition_.y, basePosition_.z + currentRadius * std::sin(theta2) };
            lineRegion->AddInstance(p1, p2, color);
            Vector3 p3 = { basePosition_.x + currentRadius * std::cos(theta1), basePosition_.y + currentRadius * std::sin(theta1), basePosition_.z };
            Vector3 p4 = { basePosition_.x + currentRadius * std::cos(theta2), basePosition_.y + currentRadius * std::sin(theta2), basePosition_.z };
            lineRegion->AddInstance(p3, p4, color);
            Vector3 p5 = { basePosition_.x, basePosition_.y + currentRadius * std::cos(theta1), basePosition_.z + currentRadius * std::sin(theta1) };
            Vector3 p6 = { basePosition_.x, basePosition_.y + currentRadius * std::cos(theta2), basePosition_.z + currentRadius * std::sin(theta2) };
            lineRegion->AddInstance(p5, p6, color);
        }
    }

    if (currentPhase_ == Phase::FinalExplosion) {
        Vector4 color = { 1.0f, 0.0f, 0.0f, 1.0f }; 

        const int segments = 16;
        float radius = finalExplosionSphere_.radius;
        const Vector3& center = finalExplosionSphere_.center;

        for (int i = 0; i < segments; ++i) {
            float theta1 = (2.0f * 3.14159265f * i) / segments;
            float theta2 = (2.0f * 3.14159265f * (i + 1)) / segments;
            Vector3 p1 = { center.x + radius * std::cos(theta1), center.y + radius * std::sin(theta1), center.z };
            Vector3 p2 = { center.x + radius * std::cos(theta2), center.y + radius * std::sin(theta2), center.z };
            lineRegion->AddInstance(p1, p2, color);
            Vector3 p3 = { center.x + radius * std::cos(theta1), center.y, center.z + radius * std::sin(theta1) };
            Vector3 p4 = { center.x + radius * std::cos(theta2), center.y, center.z + radius * std::sin(theta2) };
            lineRegion->AddInstance(p3, p4, color);
            Vector3 p5 = { center.x, center.y + radius * std::cos(theta1), center.z + radius * std::sin(theta1) };
            Vector3 p6 = { center.x, center.y + radius * std::cos(theta2), center.z + radius * std::sin(theta2) };
            lineRegion->AddInstance(p5, p6, color);
        }
    }
}

void EnemyStompEffects::Draw(IrufemiEngine* engine) {
    if (!engine) return;
    
    // 予兆は本体の爆発エフェクトがアクティブでなくても描画する
    DrawBodyTelegraph(engine);

    if (!isActive_) return;
    
    if (currentPhase_ == Phase::Expanding || currentPhase_ == Phase::KeepAndWarning) {
        explosionObj_->Draw();
        ringObj_->Draw();
    }
    
    if (gpuParticleSystem_) {
        gpuParticleSystem_->SyncBeforeDraw();
        gpuParticleSystem_->Draw();
    }
}
