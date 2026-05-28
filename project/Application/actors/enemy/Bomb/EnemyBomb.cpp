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

        float progress = telegraphTimer_ / telegraphDuration_;
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        // 予告線と爆弾コアの動的カラー計算
        Vector4 telegraphColor;
        if (progress < telegraphBlinkThreshold_) {
            // 黄色 (1.0, 1.0, 0.0, 0.5) から 赤色 (1.0, 0.0, 0.0, 0.5) への変化
            float k = progress / telegraphBlinkThreshold_;
            telegraphColor = { 1.0f, 1.0f - k, 0.0f, 0.5f };

            if (bombCoreParamsData_) {
                // コアも徐々に赤みを帯び、パルスも少しずつ早くなる
                bombCoreParamsData_->coreColor = { 1.0f, 0.8f - k * 0.6f, 0.3f - k * 0.3f, 1.0f };
                bombCoreParamsData_->pulseSpeed = 10.0f + k * 30.0f;
            }
        } else {
            // 赤色 (1.0, 0.0, 0.0, 0.5) と 白色 (1.0, 1.0, 1.0, 0.8) の高速点滅
            float blinkTime = telegraphTimer_ - (telegraphDuration_ * telegraphBlinkThreshold_);
            float blink = std::sin(blinkTime * telegraphBlinkFrequency_) * 0.5f + 0.5f;
            telegraphColor = { 1.0f, blink, blink, 0.5f + blink * 0.3f };

            if (bombCoreParamsData_) {
                // コアもシンクロして点滅させ、パルススピードを最大にする
                bombCoreParamsData_->coreColor = { 1.0f, 0.2f + blink * 0.8f, blink, 1.0f };
                bombCoreParamsData_->pulseSpeed = 60.0f;
            }
        }

        telegraphObjX_->SetColor(telegraphColor);
        telegraphObjZ_->SetColor(telegraphColor);

        // 予告線の拡大に合わせて、爆弾のコア（球体）も膨張させる（1.5f から 3.5f へ拡大）
        float sphereScale = 1.5f + progress * 2.0f;
        bombSphere_->SetScale({ sphereScale, sphereScale, sphereScale });

        bombSphere_->Update();

        // 予告線の太さをだんだん太くして、最終的に実際の爆発の太さ（explosionThickness_）に合わせる
        float thickness = 2.0f + progress * (explosionThickness_ - 2.0f);

        // X軸方向の予告線
        telegraphObjX_->SetScale({ explosionLength_, telegraphHeight_, thickness });
        Vector3 posX = targetPos_;
        posX.y += telegraphHeight_ * 0.5f; // 地面に埋まらないように高さの半分だけ浮かせる
        telegraphObjX_->SetPosition(posX);
        telegraphObjX_->SetRotate({ 0, 0, 0 });
        telegraphObjX_->Update();

        // Z軸方向の予告線
        telegraphObjZ_->SetScale({ thickness, telegraphHeight_, explosionLength_ });
        Vector3 posZ = targetPos_;
        posZ.y += telegraphHeight_ * 0.5f; // こちらも半分だけ浮かせる
        telegraphObjZ_->SetPosition(posZ);
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
        float flameHeight = explosionThickness_ * 2.5f; // 炎の高さ

        // X軸方向の攻撃
        Vector3 posX = targetPos_;
        // 地面と交差する幅を実際の太さ（explosionThickness_）に一致させるため、中心Y座標は浮かさずtargetPos_.yに揃える
        attackCylinderX_->SetPosition(posX);
        // CylinderはデフォルトでY軸向きなので、X軸向きにするにはZ軸で-90度回転
        attackCylinderX_->SetRotate({ 0, 0, -Math::PI / 2.0f }); 
        // ローカルX軸 = ワールドY軸(高さ), ローカルY軸 = ワールドX軸(長さ), ローカルZ軸 = ワールドZ軸(厚み)
        attackCylinderX_->SetScale({ flameHeight, explosionLength_, explosionThickness_ });
        attackCylinderX_->Update();
 
        // Z軸方向の攻撃
        Vector3 posZ = targetPos_;
        // こちらも同様に中心Y座標は浮かさずtargetPos_.yに揃える
        attackCylinderZ_->SetPosition(posZ);
        // Z軸向きにするにはX軸で90度回転
        attackCylinderZ_->SetRotate({ Math::PI / 2.0f, 0, 0 });
        // ローカルX軸 = ワールドX軸(厚み), ローカルY軸 = ワールドZ軸(長さ), ローカルZ軸 = ワールド-Y軸(高さ)
        attackCylinderZ_->SetScale({ explosionThickness_, explosionLength_, flameHeight });
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
        gpuParticle_->Clear();
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
        engine->SetCull(PSOManager::CullMode::None); // カリング無効化
        
        // ビルなどの不透明オブジェクト描画後に描画するためUIキュー（Draw(true)）を使用
        if (telegraphObjX_) telegraphObjX_->Draw(true);
        if (telegraphObjZ_) telegraphObjZ_->Draw(true);

        engine->SetBlend(BlendMode::kBlendModeNormal);
        engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
        engine->SetCull(PSOManager::CullMode::Back); // カリング有効化
    }

    if (state_ == State::Exploding) {
        // カスタムシェーダーで描画
        attackCylinderX_->SetCustomPSO(engine->GetPSOManager()->GetPSO("ExplosionFlame", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
        attackCylinderZ_->SetCustomPSO(engine->GetPSOManager()->GetPSO("ExplosionFlame", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
        
        if (explosionParamsResource_) {
            attackCylinderX_->SetCustomCBVAddress(explosionParamsResource_->GetGPUVirtualAddress());
            attackCylinderZ_->SetCustomCBVAddress(explosionParamsResource_->GetGPUVirtualAddress());
        }

        // ビルなどの不透明オブジェクト描画後に描画するためUIキュー（Draw(true)）を使用
        if (attackCylinderX_) attackCylinderX_->Draw(true);
        if (attackCylinderZ_) attackCylinderZ_->Draw(true);

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

    // 見た目のシリンダーの回転状態に関わらず、地面に沿った正確な当たり判定（ボックス）を生成する
    
    // X軸方向のOBB
    OBB obbX;
    obbX.center = targetPos_;
    obbX.center.y += explosionThickness_ * 0.5f; // OBBを地面から厚みの半分だけ浮かせる（プレイヤーに当たる高さ）
    obbX.orientations[0] = {1.0f, 0.0f, 0.0f}; // ローカルX (ワールドX)
    obbX.orientations[1] = {0.0f, 1.0f, 0.0f}; // ローカルY (ワールドY)
    obbX.orientations[2] = {0.0f, 0.0f, 1.0f}; // ローカルZ (ワールドZ)
    // sizeは各軸の「半径（半分）」
    obbX.size = { explosionLength_ * 0.5f, explosionThickness_ * 0.5f, explosionThickness_ * 0.5f };
    obbs.push_back(obbX);

    // Z軸方向のOBB
    OBB obbZ;
    obbZ.center = targetPos_;
    obbZ.center.y += explosionThickness_ * 0.5f;
    obbZ.orientations[0] = {1.0f, 0.0f, 0.0f};
    obbZ.orientations[1] = {0.0f, 1.0f, 0.0f};
    obbZ.orientations[2] = {0.0f, 0.0f, 1.0f};
    obbZ.size = { explosionThickness_ * 0.5f, explosionThickness_ * 0.5f, explosionLength_ * 0.5f };
    obbs.push_back(obbZ);

    return obbs;
}
