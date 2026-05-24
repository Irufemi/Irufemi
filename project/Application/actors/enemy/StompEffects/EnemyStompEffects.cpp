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

    finalExplosionObj_ = std::make_unique<PrimitiveObjects3DClass>();
    finalExplosionObj_->Initialize(PrimitiveType::Sphere, "resources/uvChecker.png");

    if (engine) {
        explosionParamsResource_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(ExplosionParams));
        explosionParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&explosionParamsData_));
        if (explosionParamsData_) {
            *explosionParamsData_ = ExplosionParams();
            explosionParamsData_->edgeColor = { 1.0f, 0.2f, 0.0f, 1.0f };
            explosionParamsData_->midColor = { 1.0f, 0.5f, 0.0f, 1.0f };
            explosionParamsData_->coreColor = { 1.0f, 1.0f, 0.8f, 1.0f };
            explosionParamsData_->speed = 5.0f;
            explosionParamsData_->intensity = 5.0f;
            explosionParamsData_->noiseScale = 2.0f;
            explosionParamsData_->erosion = 0.0f;
        }

        finalExplosionObj_->SetCustomPSO(engine->GetPSOManager()->GetPSO(
            "StompExplosion", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));

        bodyTelegraphObj_ = std::make_unique<PrimitiveObjects3DClass>();
        bodyTelegraphObj_->Initialize(PrimitiveType::Plane, "resources/whiteTexture.png");
        bodyTelegraphObj_->GetMaterial().enableLighting = false; // ライティング無効
        bodyTelegraphObj_->SetCastShadows(false); // 影を落とさない
        // AOEWarningシェーダー（加算・半透明など）を適用
        bodyTelegraphObj_->SetCustomPSO(engine->GetPSOManager()->GetPSO(
            "AOEWarning", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
        
        finalExplosionObj_->SetCustomCBVAddress(explosionParamsResource_->GetGPUVirtualAddress());
    }

    // GPUParticleSystemの初期化
    gpuParticleSystem_ = std::make_unique<GPUParticleSystem>();
    gpuParticleSystem_->Initialize("resources/circle.png");
    gpuParticleSystem_->SetBlend(BlendMode::kBlendModeAdd);
    gpuParticleSystem_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    gpuParticleSystem_->SetCull(PSOManager::CullMode::None);
    gpuParticleSystem_->SetEmit(false);
    
    // パーティクルの基本設定 (爆発らしくするための調整)
    gpuParticleSystem_->SetParticleLife(1.0f, 2.5f); // 少し長めに残して余韻を作る
    // サイズ: 小さく発生して、煙/炎として大きく膨張していくようにする
    gpuParticleSystem_->SetParticleScale({ 2.0f, 2.0f, 2.0f }, { 4.0f, 4.0f, 4.0f }, { 8.0f, 8.0f, 8.0f }, { 16.0f, 16.0f, 16.0f });
    // 物理挙動: 急激に減速し、熱気で上に舞い上がるようにする
    gpuParticleSystem_->SetDamping(0.06f);  // 高い空気抵抗（初速は早いがすぐ減速する）
    gpuParticleSystem_->SetGravity(-10.0f); // マイナス重力で上に昇る
    
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
    finalExplosionTransform_.scale = { params_.ringMaxRadius, 0.01f, params_.ringMaxRadius };
    UpdateFinalExplosionSphere();
}

void EnemyStompEffects::Cancel() {
    isActive_ = false;
    isBodyTelegraphActive_ = false;
    currentPhase_ = Phase::Finished;
    if (gpuParticleSystem_) {
        gpuParticleSystem_->Clear();
    }
}

void EnemyStompEffects::Update(float deltaTime) {
    if (!isActive_) return;

    if (gpuParticleSystem_) {
        gpuParticleSystem_->Update();
    }

    globalTimer_ += deltaTime;
    phaseTimer_ += deltaTime;

    if (currentPhase_ == Phase::Expanding || currentPhase_ == Phase::KeepAndWarning) {
        float totalWarningTime = params_.ringExpandDuration + params_.ringKeepDuration;
        float t = (std::min)(1.0f, globalTimer_ / totalWarningTime);
        
        float easeIn = EaseInQuad(t);
        float currentScale = Lerp(1.0f, params_.ringMaxRadius, easeIn);
        
        explosionTransform_.scale = { currentScale, currentScale, currentScale };
        explosionTransform_.translate.y = basePosition_.y + params_.ringGroundOffset;
        
        Vector4 colorBase = { 1.0f, 0.4f, 0.0f, 0.2f }; 
        Vector4 colorWarning = { 1.0f, 0.0f, 0.0f, 0.6f };
        Vector4 currentColor;
        currentColor.x = Lerp(colorBase.x, colorWarning.x, t);
        currentColor.y = Lerp(colorBase.y, colorWarning.y, t);
        currentColor.z = Lerp(colorBase.z, colorWarning.z, t);
        currentColor.w = Lerp(colorBase.w, colorWarning.w, t);
        
        float pulse = (std::sin(globalTimer_ * 15.0f * t) * 0.5f + 0.5f);
        currentColor.w += pulse * 0.2f * t; 

        explosionObj_->SetColor(currentColor);
    }

    switch (currentPhase_) {
    case Phase::Expanding:
    {
        float t = (std::min)(1.0f, phaseTimer_ / params_.ringExpandDuration);
        float currentRadius = Lerp(1.0f, params_.ringMaxRadius, t);

        ringTransform_.scale = { currentRadius, params_.ringHeight, currentRadius };
        ringObj_->SetColor(params_.ringColorNormal);

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

        Vector4 c;
        c.x = Lerp(params_.ringColorNormal.x, params_.ringColorWarning.x, t);
        c.y = Lerp(params_.ringColorNormal.y, params_.ringColorWarning.y, t);
        c.z = Lerp(params_.ringColorNormal.z, params_.ringColorWarning.z, t);
        c.w = Lerp(params_.ringColorNormal.w, params_.ringColorWarning.w, t);
        ringObj_->SetColor(c);

        if (t >= 1.0f) {
            currentPhase_ = Phase::FinalExplosion;
            phaseTimer_ = 0.0f;
            hasDealtFinalDamage_ = false;
            
            if (gpuParticleSystem_) {
                float burstRadius = 5.0f; // より中心から密度高く発生させる
                gpuParticleSystem_->SetHemisphereEmitter(
                    Vector3{ basePosition_.x, basePosition_.y + params_.ringGroundOffset + 2.0f, basePosition_.z },
                    burstRadius,
                    0, 
                    1.0f 
                );
                // Dampingが強いので、初速はさらにドカンと速くする
                gpuParticleSystem_->SetVelocity(250.0f); 
                gpuParticleSystem_->SetSpread(1.0f); 
                // 少し時間差で2回に分けて放出するなどを想定して、多めに出す
                gpuParticleSystem_->Emit(4000);
            }
            
            UpdateFinalExplosionSphere();
            float initR = params_.ringMaxRadius;
            float initH = 0.01f;
            finalExplosionObj_->SetScale(Vector3{ initR, initH, initR });
            finalExplosionObj_->SetPosition(Vector3{
                basePosition_.x,
                basePosition_.y + params_.ringGroundOffset,
                basePosition_.z
            });
            UpdateFinalExplosionSphere();
        }
    }
    break;

    case Phase::FinalExplosion:
    {
        float t = (std::min)(1.0f, phaseTimer_ / params_.finalExplosionDuration);

        float easeOutH = EaseOutCubic(t);
        float currentH = Lerp(0.01f, params_.finalExplosionMaxHeight, easeOutH);
        float easeOutR = EaseOutCubic(t);
        float currentR = Lerp(params_.ringMaxRadius, params_.finalExplosionMaxRadius, easeOutR);

        finalExplosionObj_->SetScale(Vector3{ currentR, currentR, currentR });
        
        finalExplosionObj_->SetPosition(Vector3{
            basePosition_.x,
            basePosition_.y + params_.ringGroundOffset,
            basePosition_.z
        });

        if (explosionParamsData_) {
            float flashT = 1.0f - easeOutR; 
            explosionParamsData_->intensity = Lerp(0.0f, 8.0f, flashT); 
            explosionParamsData_->speed = Lerp(1.0f, 5.0f, flashT);
            explosionParamsData_->noiseScale = Lerp(1.2f, 0.6f, t);
            explosionParamsData_->edgeColor = Vector4{ 1.0f, 0.05f, 0.0f, 1.0f }; 
            explosionParamsData_->midColor  = Vector4{ 1.0f, 0.4f, 0.0f, 1.0f };  
            explosionParamsData_->coreColor = Vector4{ 1.0f, 0.8f, 0.1f, 1.0f };  
            float erosionT = (t < 0.4f) ? 0.0f : (t - 0.4f) / 0.6f;
            explosionParamsData_->erosion = Lerp(0.0f, 2.0f, erosionT); 
            
            // Raymarching用の球体情報
            explosionParamsData_->sphereCenter = finalExplosionObj_->GetCenter();
            explosionParamsData_->sphereRadius = currentR * 0.5f; // currentRは直径(スケール)として扱っているので半分が半径
        }

        UpdateFinalExplosionSphere();

        if (t >= 1.0f) {
            currentPhase_ = Phase::Finished;
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
    if (finalExplosionObj_) {
        finalExplosionObj_->Update();
    }
}

void EnemyStompEffects::UpdateFinalExplosionSphere() {
    finalExplosionSphere_.center = finalExplosionObj_->GetCenter();
    Vector3 scale = finalExplosionObj_->GetTransform().transform.scale;
    finalExplosionSphere_.radius = scale.x * 0.5f;
}

bool EnemyStompEffects::IsExplosionDamageActive() const {
    if (globalTimer_ >= params_.explosionDuration) return false;
    float t = globalTimer_ / params_.explosionDuration;
    return t < params_.explosionDamageActiveTime;
}

float EnemyStompEffects::GetExplosionRadius() const {
    float t = (std::min)(1.0f, globalTimer_ / params_.explosionDuration);
    float easeOut = EaseOutQuad(t);
    return Lerp(1.0f, params_.explosionMaxRadius, easeOut);
}

void EnemyStompEffects::StartBodyTelegraph(const Vector3& pos, float radius) {
    isBodyTelegraphActive_ = true;
    bodyTelegraphTransform_.scale = { radius, radius, 1.0f }; // PlaneはXY平面なのでXとYをスケーリング
    bodyTelegraphTransform_.rotate = { std::numbers::pi_v<float> / 2.0f, 0.0f, 0.0f }; // X軸で90度寝かせる
    bodyTelegraphTransform_.translate = pos; // 呼び出し元で指定された高さ（Y）をそのまま使う
    
    // UV Transformを利用してシェーダーにパラメータを渡す
    // _11: shapeType (0 = 円形)
    // _12: warningRatio (0.0 から開始)
    bodyTelegraphObj_->GetMaterial().uvTransform.m[0][0] = 0.0f; 
    bodyTelegraphObj_->GetMaterial().uvTransform.m[0][1] = 0.0f;
    
    // 赤色（少しオレンジを混ぜておく）、アルファはシェーダー内で調整される
    bodyTelegraphObj_->SetColor({ 1.0f, 0.1f, 0.0f, 0.8f });
    bodyTelegraphObj_->SetTransform(bodyTelegraphTransform_);
    bodyTelegraphObj_->Update();
}

void EnemyStompEffects::UpdateBodyTelegraph(const Vector3& pos, float warningRatio) {
    if (!isBodyTelegraphActive_ || !bodyTelegraphObj_) return;
    
    bodyTelegraphTransform_.translate = pos;
    bodyTelegraphObj_->SetTransform(bodyTelegraphTransform_);
    
    // パラメータ更新
    bodyTelegraphObj_->GetMaterial().uvTransform.m[0][0] = 0.0f; // 円形
    bodyTelegraphObj_->GetMaterial().uvTransform.m[0][1] = warningRatio;
    
    bodyTelegraphObj_->Update();
}

void EnemyStompEffects::StopBodyTelegraph() {
    isBodyTelegraphActive_ = false;
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
    if (currentPhase_ == Phase::FinalExplosion) {
        finalExplosionObj_->Draw();
    }
    
    if (gpuParticleSystem_) {
        gpuParticleSystem_->SyncBeforeDraw();
        gpuParticleSystem_->Draw();
    }
}
