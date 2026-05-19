#include "EnemyBomb.h"
#include "Core/Math/Math.h"
#include "IrufemiEngine.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include <cmath>
#include <algorithm>

EnemyBomb::~EnemyBomb() {
    if (lightningParamsResource_ && engine_ && engine_->GetDirectXCommon()) {
        engine_->GetDirectXCommon()->ReleaseAfterFence(lightningParamsResource_);
    }
}

void EnemyBomb::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    // 爆弾本体の見た目
    bombSphere_ = std::make_unique<SphereClass>();
    bombSphere_->Initialize();
    bombSphere_->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });

    // 予告線X軸
    telegraphObjX_ = std::make_unique<ObjClass>();
    telegraphObjX_->Initialize("sample/block.obj");
    telegraphObjX_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });
    telegraphObjX_->SetCastShadows(false);

    // 予告線Z軸
    telegraphObjZ_ = std::make_unique<ObjClass>();
    telegraphObjZ_->Initialize("sample/block.obj");
    telegraphObjZ_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });
    telegraphObjZ_->SetCastShadows(false);

    // 攻撃判定X軸
    attackCylinderX_ = std::make_shared<CylinderClass>();
    attackCylinderX_->Initialize(false, false);
    attackCylinderX_->SetColor({ 1.0f, 0.0f, 0.0f, 0.8f });
    attackCylinderX_->SetCastShadows(false);

    // 攻撃判定Z軸
    attackCylinderZ_ = std::make_shared<CylinderClass>();
    attackCylinderZ_->Initialize(false, false);
    attackCylinderZ_->SetColor({ 1.0f, 0.0f, 0.0f, 0.8f });
    attackCylinderZ_->SetCastShadows(false);

    if (engine) {
        lightningParamsResource_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(LightningParams));
        lightningParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightningParamsData_));
        if (lightningParamsData_) {
            *lightningParamsData_ = LightningParams();
            lightningParamsData_->color = { 1.0f, 0.3f, 0.1f, 1.0f };
            lightningParamsData_->coreColor = { 1.0f, 0.9f, 0.3f, 1.0f };
            lightningParamsData_->intensity = 5.0f;
            lightningParamsData_->noiseThreshold = 0.2f;
            lightningParamsData_->coreIntensity = 4.0f;
            lightningParamsData_->coreScale = 1.5f;
            lightningParamsData_->speed = 5.0f;
            lightningParamsData_->noiseScale = 4.0f;
        }

        gpuParticle_ = std::make_unique<GPUParticleSystem>();
        gpuParticle_->Initialize("resources/circle.png");
        gpuParticle_->SetColor({ 1.0f, 0.3f, 0.0f, 1.0f });
        gpuParticle_->SetEmit(false);
    }

    state_ = State::Idle;
    isExpired_ = false;
}

void EnemyBomb::Throw(const Vector3& startPos, const Vector3& targetPos) {
    startPos_ = startPos;
    targetPos_ = targetPos;
    flightTimer_ = 0.0f;
    telegraphTimer_ = 0.0f;
    explodeTimer_ = 0.0f;
    state_ = State::Flying;
    isExpired_ = false;

    // 着弾位置の高さを少し上にする（地面にめり込まないように）
    targetPos_.y = targetPos_.y > 0.1f ? targetPos_.y : 0.1f;
}

void EnemyBomb::Update() {
    float deltaTime = 1.0f / 60.0f;

    switch (state_) {
    case State::Flying: {
        flightTimer_ += deltaTime;
        float t = flightTimer_ / flightDuration_;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        
        // XY平面での放物線補間
        Vector3 currentPos = {
            startPos_.x + (targetPos_.x - startPos_.x) * t,
            startPos_.y + (targetPos_.y - startPos_.y) * t,
            startPos_.z + (targetPos_.z - startPos_.z) * t
        };
        
        // 放物線の高さを加算
        float parabola = std::sin(t * Math::PI);
        currentPos.y += throwHeight_ * parabola;

        bombSphere_->SetCenter(currentPos);
        bombSphere_->SetRadius(1.5f);
        bombSphere_->Update();

        if (t >= 1.0f) {
            state_ = State::Telegraphing;
        }
        break;
    }
    case State::Telegraphing: {
        telegraphTimer_ += deltaTime;

        // 爆弾自体は点滅等の処理を入れる
        float blink = std::abs(std::sin(telegraphTimer_ * 15.0f));
        bombSphere_->SetColor({ 1.0f, 0.2f + 0.8f * blink, 0.2f + 0.8f * blink, 1.0f });
        bombSphere_->Update();

        // 予告線の太さをだんだん太くする
        float thickness = 0.5f + (telegraphTimer_ / telegraphDuration_) * 1.5f;

        // X軸方向の予告線
        telegraphTransformX_.scale = { explosionLength_, thickness, thickness };
        telegraphTransformX_.translate = targetPos_;
        telegraphTransformX_.rotate = { 0, 0, 0 };
        telegraphObjX_->SetTransform(telegraphTransformX_);
        telegraphObjX_->Update();

        // Z軸方向の予告線
        telegraphTransformZ_.scale = { thickness, thickness, explosionLength_ };
        telegraphTransformZ_.translate = targetPos_;
        telegraphTransformZ_.rotate = { 0, 0, 0 };
        telegraphObjZ_->SetTransform(telegraphTransformZ_);
        telegraphObjZ_->Update();

        if (telegraphTimer_ >= telegraphDuration_) {
            state_ = State::Exploding;
        }
        break;
    }
    case State::Exploding: {
        explodeTimer_ += deltaTime;
        
        // Cylinderは長さ=height、太さ=radiusなので、X軸とZ軸に沿わせるために回転させる
        // X軸方向の攻撃
        attackCylinderX_->SetCenter(targetPos_);
        // CylinderClassはデフォルトでY軸向きなので、X軸向きにするにはZ軸で-90度回転
        attackCylinderX_->SetRotate({ 0, 0, -Math::PI / 2.0f }); 
        attackCylinderX_->SetRadius(explosionThickness_ * 0.5f);
        attackCylinderX_->SetHeight(explosionLength_);
        attackCylinderX_->Update();

        // Z軸方向の攻撃
        attackCylinderZ_->SetCenter(targetPos_);
        // Z軸向きにするにはX軸で90度回転
        attackCylinderZ_->SetRotate({ Math::PI / 2.0f, 0, 0 });
        attackCylinderZ_->SetRadius(explosionThickness_ * 0.5f);
        attackCylinderZ_->SetHeight(explosionLength_);
        attackCylinderZ_->Update();

        // パーティクル放出
        if (gpuParticle_) {
            gpuParticle_->SetParticleLife(0.3f, 0.6f);
            gpuParticle_->SetParticleScale(
                { 1.0f, 1.0f, 1.0f }, { 3.0f, 3.0f, 3.0f },
                { 0.1f, 0.1f, 0.1f }, { 0.5f, 0.5f, 0.5f }
            );

            // 十字方向に放射状に出す
            gpuParticle_->SetSphereEmitter(targetPos_, explosionLength_ * 0.1f, 200, 0.01f);
            gpuParticle_->SetEmit(true);
            gpuParticle_->Update();
        }

        if (explodeTimer_ >= explodeDuration_) {
            state_ = State::Done;
            isExpired_ = true;
            if (gpuParticle_) {
                gpuParticle_->SetEmit(false);
                gpuParticle_->Update();
            }
        }
        break;
    }
    case State::Done:
    default:
        break;
    }
}

void EnemyBomb::Draw(IrufemiEngine* engine) {
    if (!engine) return;

    if (state_ == State::Flying || state_ == State::Telegraphing) {
        if (bombSphere_) bombSphere_->Draw();
    }

    if (state_ == State::Telegraphing) {
        if (telegraphObjX_) telegraphObjX_->Draw();
        if (telegraphObjZ_) telegraphObjZ_->Draw();
    }

    if (state_ == State::Exploding) {
        // カスタムシェーダーで描画
        attackCylinderX_->SetCustomPSO(engine->GetPSOManager()->GetPSO("LightningCrawl", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
        attackCylinderZ_->SetCustomPSO(engine->GetPSOManager()->GetPSO("LightningCrawl", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
        
        if (lightningParamsResource_) {
            attackCylinderX_->SetCustomCBVAddress(lightningParamsResource_->GetGPUVirtualAddress());
            attackCylinderZ_->SetCustomCBVAddress(lightningParamsResource_->GetGPUVirtualAddress());
        }

        engine->SetBlend(BlendMode::kBlendModeAdd);
        engine->SetDepthWrite(PSOManager::DepthWrite::Disable);
        engine->SetCull(PSOManager::CullMode::None);

        if (attackCylinderX_) attackCylinderX_->Draw();
        if (attackCylinderZ_) attackCylinderZ_->Draw();

        engine->SetBlend(BlendMode::kBlendModeNormal);
        engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
        engine->SetCull(PSOManager::CullMode::Back);

        if (gpuParticle_) {
            gpuParticle_->Draw();
        }
    }
}

std::vector<OBB> EnemyBomb::GetOBBs() const {
    std::vector<OBB> obbs;
    if (state_ != State::Exploding) {
        return obbs;
    }

    // X軸方向のOBB
    OBB obbX;
    obbX.center = attackCylinderX_->GetCenter();
    obbX.orientations[0] = attackCylinderX_->GetRight();
    obbX.orientations[1] = attackCylinderX_->GetDirection();
    obbX.orientations[2] = attackCylinderX_->GetUp();
    obbX.size = { explosionThickness_ * 0.5f, explosionThickness_ * 0.5f, explosionLength_ * 0.5f };
    obbs.push_back(obbX);

    // Z軸方向のOBB
    OBB obbZ;
    obbZ.center = attackCylinderZ_->GetCenter();
    obbZ.orientations[0] = attackCylinderZ_->GetRight();
    obbZ.orientations[1] = attackCylinderZ_->GetDirection();
    obbZ.orientations[2] = attackCylinderZ_->GetUp();
    obbZ.size = { explosionThickness_ * 0.5f, explosionThickness_ * 0.5f, explosionLength_ * 0.5f };
    obbs.push_back(obbZ);

    return obbs;
}
