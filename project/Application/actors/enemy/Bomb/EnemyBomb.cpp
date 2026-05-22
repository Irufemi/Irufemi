#include "EnemyBomb.h"
#include "Core/Math/Math.h"
#include "IrufemiEngine.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include <cmath>
#include <algorithm>

EnemyBomb::~EnemyBomb() {
    if (explosionParamsResource_ && engine_ && engine_->GetDirectXCommon()) {
        engine_->GetDirectXCommon()->ReleaseAfterFence(explosionParamsResource_);
    }
    if (bombCoreParamsResource_ && engine_ && engine_->GetDirectXCommon()) {
        engine_->GetDirectXCommon()->ReleaseAfterFence(bombCoreParamsResource_);
    }
}

void EnemyBomb::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    // 爆弾本体の見た目
    bombSphere_ = std::make_shared<PrimitiveObjects3DClass>();
    bombSphere_->Initialize(PrimitiveType::Sphere);
    bombSphere_->SetCastShadows(false);

    // 予告線X軸
    telegraphObjX_ = std::make_shared<PrimitiveObjects3DClass>();
    telegraphObjX_->Initialize(PrimitiveType::Cube, "resources/whiteTexture.png");
    telegraphObjX_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });
    telegraphObjX_->SetCastShadows(false);

    // 予告線Z軸
    telegraphObjZ_ = std::make_shared<PrimitiveObjects3DClass>();
    telegraphObjZ_->Initialize(PrimitiveType::Cube, "resources/whiteTexture.png");
    telegraphObjZ_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });
    telegraphObjZ_->SetCastShadows(false);

    // 攻撃判定X軸
    attackCylinderX_ = std::make_shared<PrimitiveObjects3DClass>();
    attackCylinderX_->Initialize(PrimitiveType::Cylinder);
    attackCylinderX_->SetColor({ 1.0f, 0.0f, 0.0f, 0.8f });
    attackCylinderX_->SetCastShadows(false);

    // 攻撃判定Z軸
    attackCylinderZ_ = std::make_shared<PrimitiveObjects3DClass>();
    attackCylinderZ_->Initialize(PrimitiveType::Cylinder);
    attackCylinderZ_->SetColor({ 1.0f, 0.0f, 0.0f, 0.8f });
    attackCylinderZ_->SetCastShadows(false);

    if (engine) {
        explosionParamsResource_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(ExplosionParams));
        explosionParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&explosionParamsData_));
        if (explosionParamsData_) {
            *explosionParamsData_ = ExplosionParams();
            explosionParamsData_->edgeColor = { 1.0f, 0.2f, 0.0f, 1.0f };
            explosionParamsData_->midColor = { 1.0f, 0.5f, 0.0f, 1.0f };
            explosionParamsData_->coreColor = { 1.0f, 1.0f, 0.8f, 1.0f };
            explosionParamsData_->speed = 5.0f;
            explosionParamsData_->intensity = 4.0f;
            explosionParamsData_->noiseScale = 3.0f;
            explosionParamsData_->erosion = 0.0f;
        }

        bombCoreParamsResource_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(BombCoreParams));
        bombCoreParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&bombCoreParamsData_));
        if (bombCoreParamsData_) {
            *bombCoreParamsData_ = BombCoreParams();
            bombCoreParamsData_->edgeColor = { 1.0f, 0.1f, 0.0f, 1.0f };
            bombCoreParamsData_->coreColor = { 1.0f, 0.8f, 0.3f, 1.0f };
            bombCoreParamsData_->crackColor = { 1.0f, 0.2f, 0.1f, 1.0f };
            bombCoreParamsData_->noiseScale = 2.0f;
            bombCoreParamsData_->distortion = 0.2f;
            bombCoreParamsData_->pulseSpeed = 10.0f;
            bombCoreParamsData_->intensity = 2.0f;
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

        bombSphere_->SetPosition(currentPos);
        bombSphere_->SetScale({ 1.5f, 1.5f, 1.5f });
        bombSphere_->Update();

        if (bombCoreParamsData_) {
            bombCoreParamsData_->pulseSpeed = 10.0f; // 飛翔中は比較的穏やかな明滅
        }

        if (t >= 1.0f) {
            state_ = State::Telegraphing;
        }
        break;
    }
    case State::Telegraphing: {
        telegraphTimer_ += deltaTime;

        // 爆弾自体はシェーダーパラメータでパルスを激しくする
        if (bombCoreParamsData_) {
            bombCoreParamsData_->pulseSpeed = 40.0f; // 爆発寸前は激しく明滅
        }
        bombSphere_->Update();

        // 予告線の太さをだんだん太くする
        float thickness = 0.5f + (telegraphTimer_ / telegraphDuration_) * 1.5f;

        // X軸方向の予告線
        telegraphObjX_->SetScale({ explosionLength_, thickness, thickness });
        telegraphObjX_->SetPosition(targetPos_);
        telegraphObjX_->SetRotate({ 0, 0, 0 });
        telegraphObjX_->Update();

        // Z軸方向の予告線
        telegraphObjZ_->SetScale({ thickness, thickness, explosionLength_ });
        telegraphObjZ_->SetPosition(targetPos_);
        telegraphObjZ_->SetRotate({ 0, 0, 0 });
        telegraphObjZ_->Update();

        if (telegraphTimer_ >= telegraphDuration_) {
            state_ = State::Exploding;
        }
        break;
    }
    case State::Exploding: {
        explodeTimer_ += deltaTime;
        float explodeRatio = explodeTimer_ / explodeDuration_;
        if (explosionParamsData_) {
            explosionParamsData_->erosion = explodeRatio; // 爆発の終わりに向けて浸食を進める
            explosionParamsData_->intensity = 4.0f * (1.0f - explodeRatio); // 強度を落として消えるようにする
        }
        
        // Cylinderは長さ=height、太さ=radiusなので、X軸とZ軸に沿わせるために回転させる
        // 炎が上に高く立ち昇るように、ローカルのスケールを操作して楕円柱（壁）にする
        float heightScale = 5.0f; // 炎の高さの倍率
        float currentRadius = explosionThickness_ * 0.5f;

        // X軸方向の攻撃
        Vector3 posX = targetPos_;
        posX.y += (currentRadius * heightScale) * 0.5f; // 地面にめり込まないよう上にずらす（高さの半分だけ上げる）
        attackCylinderX_->SetPosition(posX);
        // CylinderはデフォルトでY軸向きなので、X軸向きにするにはZ軸で-90度回転
        attackCylinderX_->SetRotate({ 0, 0, -Math::PI / 2.0f }); 
        // ローカルX軸 = ワールドY軸(高さ), ローカルY軸 = ワールドX軸(長さ), ローカルZ軸 = ワールドZ軸(厚み)
        attackCylinderX_->SetScale({ currentRadius * heightScale, explosionLength_, currentRadius });
        attackCylinderX_->Update();

        // Z軸方向の攻撃
        Vector3 posZ = targetPos_;
        posZ.y += (currentRadius * heightScale) * 0.5f; // こちらも高さの半分だけ上げる
        attackCylinderZ_->SetPosition(posZ);
        // Z軸向きにするにはX軸で90度回転
        attackCylinderZ_->SetRotate({ Math::PI / 2.0f, 0, 0 });
        // ローカルX軸 = ワールドX軸(厚み), ローカルY軸 = ワールドZ軸(長さ), ローカルZ軸 = ワールド-Y軸(高さ)
        attackCylinderZ_->SetScale({ currentRadius, explosionLength_, currentRadius * heightScale });
        attackCylinderZ_->Update();

        // パーティクル放出
        if (gpuParticle_) {
            gpuParticle_->SetParticleLife(0.3f, 0.6f);
            gpuParticle_->SetParticleScale(
                { 1.0f, 1.0f, 1.0f }, { 3.0f, 3.0f, 3.0f },
                { 0.1f, 0.1f, 0.1f }, { 0.5f, 0.5f, 0.5f }
            );

            // 十字ではなく「下から上へ」昇る表現を強調するため、
            // 球状に散らすのではなく、上方向(Y軸)へ向かうビームエミッターを使用する
            gpuParticle_->SetBeamEmitter(targetPos_, { 0.0f, 1.0f, 0.0f }, explosionLength_ * 0.2f, 15.0f, 0.5f, 200, 0.01f);
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

void EnemyBomb::Cancel() {
    state_ = State::Done;
    isExpired_ = true;
    if (gpuParticle_) {
        gpuParticle_->SetEmit(false);
        gpuParticle_->Update();
    }
}

void EnemyBomb::Draw(IrufemiEngine* engine) {
    if (!engine) return;

    if (state_ == State::Flying || state_ == State::Telegraphing) {
        if (bombSphere_) {
            bombSphere_->SetCustomPSO(engine->GetPSOManager()->GetPSO("BombCore", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Enable, PSOManager::CullMode::Back));
            if (bombCoreParamsResource_) {
                bombSphere_->SetCustomCBVAddress(bombCoreParamsResource_->GetGPUVirtualAddress());
            }
            bombSphere_->Draw();
        }
    }

    if (state_ == State::Telegraphing) {
        engine->SetBlend(BlendMode::kBlendModeAdd);
        engine->SetDepthWrite(PSOManager::DepthWrite::Disable);
        
        if (telegraphObjX_) telegraphObjX_->Draw();
        if (telegraphObjZ_) telegraphObjZ_->Draw();

        engine->SetBlend(BlendMode::kBlendModeNormal);
        engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
    }

    if (state_ == State::Exploding) {
        // カスタムシェーダーで描画
        attackCylinderX_->SetCustomPSO(engine->GetPSOManager()->GetPSO("ExplosionFlame", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
        attackCylinderZ_->SetCustomPSO(engine->GetPSOManager()->GetPSO("ExplosionFlame", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
        
        if (explosionParamsResource_) {
            attackCylinderX_->SetCustomCBVAddress(explosionParamsResource_->GetGPUVirtualAddress());
            attackCylinderZ_->SetCustomCBVAddress(explosionParamsResource_->GetGPUVirtualAddress());
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
