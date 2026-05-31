#include "EnemyTackleEffects.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Irufemi.h" 
#include "Core/Math/Math.h"
#include <cmath>
#include <algorithm>

void EnemyTackleEffects::Initialize(IrufemiEngine* engine) {
    // 砂煙の芯（地面を這う濃い部分）パーティクルの初期化
    rushCoreParticleSystem_ = std::make_unique<GPUParticleSystem>();
    rushCoreParticleSystem_->Initialize("resources/circle.png");
    rushCoreParticleSystem_->SetBlend(BlendMode::kBlendModeAdd);
    rushCoreParticleSystem_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    rushCoreParticleSystem_->SetCull(PSOManager::CullMode::None);
    rushCoreParticleSystem_->SetEmit(false);
    
    rushCoreParticleSystem_->SetParticleLife(0.2f, 0.4f); // 少し寿命を縮める
    rushCoreParticleSystem_->SetParticleScale({1.0f, 1.0f, 1.0f}, {1.5f, 1.5f, 1.5f}, {1.5f, 1.5f, 1.5f}, {2.0f, 2.0f, 2.0f}); // 広がりすぎないようにスケールを抑える
    rushCoreParticleSystem_->SetGravity(0.02f); // 地面に張り付くように少し下へ（重力）
    rushCoreParticleSystem_->SetDamping(0.2f); // 強い空気抵抗ですぐにその場に留まる
    rushCoreParticleSystem_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    // かなり濃い不透明度（芯の厚み）
    rushCoreParticleSystem_->SetStartColor({0.8f, 0.75f, 0.65f, 0.9f}, {0.7f, 0.65f, 0.55f, 0.8f}); 
    rushCoreParticleSystem_->SetEndColor({0.7f, 0.65f, 0.55f, 0.0f}, {0.6f, 0.55f, 0.45f, 0.0f});

    // 砂煙の外縁（上に巻き上がる薄い部分）パーティクルの初期化
    rushParticleSystem_ = std::make_unique<GPUParticleSystem>();
    rushParticleSystem_->Initialize("resources/circle.png");
    rushParticleSystem_->SetBlend(BlendMode::kBlendModeAdd); // テクスチャの黒背景を透過するため加算描画に変更
    rushParticleSystem_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    rushParticleSystem_->SetCull(PSOManager::CullMode::None);
    rushParticleSystem_->SetEmit(false);
    
    rushParticleSystem_->SetParticleLife(0.5f, 1.0f); // 少し寿命を縮める
    rushParticleSystem_->SetParticleScale({1.5f, 1.5f, 1.5f}, {2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f}, {4.5f, 4.5f, 4.5f}); // 大げさに広がりすぎないように最大スケールを半減
    rushParticleSystem_->SetGravity(-0.03f); // 上に巻き上がる
    rushParticleSystem_->SetDamping(0.05f); // ふわっと漂う
    rushParticleSystem_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    // 最初から薄く、上のほうで消える
    rushParticleSystem_->SetStartColor({0.8f, 0.75f, 0.65f, 0.3f}, {0.7f, 0.65f, 0.55f, 0.2f}); 
    rushParticleSystem_->SetEndColor({0.7f, 0.65f, 0.55f, 0.0f}, {0.6f, 0.55f, 0.45f, 0.0f});

    // 大爆発（壁激突時）パーティクルの初期化
    crashParticleSystem_ = std::make_unique<GPUParticleSystem>();
    crashParticleSystem_->Initialize("resources/circle.png");
    crashParticleSystem_->SetBlend(BlendMode::kBlendModeAdd);
    crashParticleSystem_->SetDepthWrite(PSOManager::DepthWrite::Disable);
    crashParticleSystem_->SetCull(PSOManager::CullMode::None);
    crashParticleSystem_->SetCustomPSO("StompExplosionParticle"); // 形状などは流用
    crashParticleSystem_->SetEmit(false);

    crashParticleSystem_->SetParticleLife(0.6f, 1.5f);
    crashParticleSystem_->SetParticleScale({0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {2.0f, 2.0f, 2.0f}, {3.5f, 3.5f, 3.5f});
    crashParticleSystem_->SetGravity(-0.1f);
    crashParticleSystem_->SetDamping(0.08f);
    crashParticleSystem_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});
    crashParticleSystem_->SetStartColor({1.0f, 0.9f, 0.6f, 1.0f}, {1.0f, 0.8f, 0.4f, 1.0f}); // 軽めの色味に変更
    crashParticleSystem_->SetEndColor({1.0f, 0.6f, 0.2f, 0.0f}, {0.9f, 0.4f, 0.1f, 0.0f}); // 軽めの色味に変更

    // 予告線（AOE）用
    telegraphObj_ = std::make_shared<PrimitiveObjects3DClass>();
    telegraphObj_->Initialize(PrimitiveType::Plane, "resources/whiteTexture.png");
    telegraphObj_->GetMaterial().enableLighting = false; // ライティング無効
    telegraphObj_->SetCastShadows(false);                // 影を落とさない
    if (engine) {
        telegraphObj_->SetCustomPSO(engine->GetPSOManager()->GetPSO("AOEWarning", BlendMode::kBlendModeAdd, PSOManager::DepthWrite::Off, PSOManager::CullMode::None));
        
        aoeParamsResource_ = engine->GetDirectXCommon()->CreateBufferResource(sizeof(AOEParams));
        aoeParamsResource_->Map(0, nullptr, reinterpret_cast<void**>(&aoeParamsData_));
        if (aoeParamsData_) {
            *aoeParamsData_ = AOEParams();
            aoeParamsData_->shapeType = 1; // Linear
        }
        telegraphObj_->SetCustomCBVAddress(aoeParamsResource_->GetGPUVirtualAddress());
    }
}

void EnemyTackleEffects::StartTelegraph(const Vector3& position, float rotateY, float length, float width) {
    isTelegraphActive_ = true;
    telegraphTransform_.scale = { width, length, 1.0f }; // PlaneはXY平面なのでYに奥行きを持たせる
    // 長さの半分だけ前にずらすことで、ボスの足元から前方に伸びるようにする
    float halfLength = length * 0.5f;
    Vector3 forward = { std::sin(rotateY), 0.0f, std::cos(rotateY) };
    telegraphTransform_.translate = { 
        position.x + forward.x * halfLength, 
        position.y - 2.4f, // 足元（地面）に這わせる
        position.z + forward.z * halfLength 
    };
    telegraphTransform_.rotate = { std::numbers::pi_v<float> / 2.0f, rotateY, 0.0f }; // Xで寝かせてYで回す
    telegraphObj_->SetTransform(telegraphTransform_);
    
    // Shader用パラメータ
    if (aoeParamsData_) {
        aoeParamsData_->warningRatio = 0.0f;
    }

    telegraphObj_->SetColor({ 1.0f, 0.2f, 0.0f, 1.0f }); // アルファはシェーダー制御
    telegraphObj_->Update();
}

void EnemyTackleEffects::UpdateTelegraph(const Vector3& position, float rotateY, float warningRatio) {
    if (!isTelegraphActive_ || !telegraphObj_) return;
    
    // 位置の追従（Aim中など位置・角度が変わる場合に対応）
    float length = telegraphTransform_.scale.y; // scale.yが長さ
    float halfLength = length * 0.5f;
    Vector3 forward = { std::sin(rotateY), 0.0f, std::cos(rotateY) };
    telegraphTransform_.translate = { 
        position.x + forward.x * halfLength, 
        position.y - 2.4f, // 足元（地面）に這わせる
        position.z + forward.z * halfLength 
    };
    telegraphTransform_.rotate = { std::numbers::pi_v<float> / 2.0f, rotateY, 0.0f };

    // 徐々に激しくなる点滅（ベースのカラー用）
    float blinkSpeed = Lerp(5.0f, 30.0f, warningRatio); 
    float blink = (std::sin(warningRatio * blinkSpeed) + 1.0f) * 0.5f; 
    
    // Shader用パラメータ
    if (aoeParamsData_) {
        aoeParamsData_->warningRatio = warningRatio;
    }

    // 赤＋少し黄色を混ぜて危険色を強調
    telegraphObj_->SetColor({ 1.0f, Lerp(0.2f, 0.5f, blink), 0.0f, 1.0f });
    telegraphObj_->SetTransform(telegraphTransform_);
    telegraphObj_->Update();
}

void EnemyTackleEffects::StopTelegraph() {
    isTelegraphActive_ = false;
    telegraphTransform_.scale = { 0.0f, 0.0f, 0.0f };
    if (telegraphObj_) {
        telegraphObj_->SetTransform(telegraphTransform_);
    }
}

void EnemyTackleEffects::DrawTelegraph(IrufemiEngine* engine) {
    if (isTelegraphActive_ && telegraphObj_ && engine) {
        telegraphObj_->Draw();
    }
}

void EnemyTackleEffects::FireRushWave(const Vector3& position) {
    TackleWave wave;
    wave.transform.translate = position;
    wave.transform.translate.y = position.y - 2.5f; // 足元に調整
    wave.transform.rotate = { 0, 0, 0 };
    wave.transform.scale = { kRushWaveStartScale, kRushWaveStartScale, kRushWaveStartScale };
    
    wave.timer = 0.0f;
    wave.maxLife = kRushWaveLife;
    wave.isCrash = false;
    wave.color = { 0.8f, 0.7f, 0.5f, kRushWaveStartAlpha }; // 砂煙っぽい色

    waves_.push_back(wave);

    if (rushParticleSystem_) {
        // 巻き上がる薄い煙（上方向メインで横への広がりを抑える）
        rushParticleSystem_->SetHemisphereEmitter(wave.transform.translate, 1.0f, 0, 1.0f); // 発生源を少し狭める
        rushParticleSystem_->SetVelocity(1.0f); // 横の勢いを抑える
        rushParticleSystem_->SetSpread(0.5f);   // 広がりを抑える
        rushParticleSystem_->Emit(20);
    }
    
    if (rushCoreParticleSystem_) {
        // 地面を這う濃い煙（横へ広がりすぎないように初速を抑える）
        rushCoreParticleSystem_->SetRingEmitter(wave.transform.translate, 0.5f, 1.0f, 0, 1.0f); // 厚みを減らす
        rushCoreParticleSystem_->SetVelocity(1.5f); // 横方向への初速を大幅に落とす（4.0f -> 1.5f）
        rushCoreParticleSystem_->Emit(40); // 密度は維持しつつ少し減らす
    }
}

void EnemyTackleEffects::FireCrashWave(const Vector3& position) {
    TackleWave wave;
    wave.transform.translate = position;
    wave.transform.translate.y = position.y - 2.4f; // 足元（地面）に配置する
    wave.transform.rotate = { 0, 0, 0 };
    wave.transform.scale = { kCrashWaveStartScale, 0.01f, kCrashWaveStartScale };
    wave.timer = 0.0f;
    wave.maxLife = kCrashWaveLife;
    wave.isCrash = true;
    wave.color = { 1.0f, 0.4f, 0.1f, kCrashWaveStartAlpha }; // 激しい爆発の色（オレンジ）

    waves_.push_back(wave);

    if (crashParticleSystem_) {
        // 中心部分の初期爆発
        crashParticleSystem_->SetHemisphereEmitter(position, 1.5f, 0, 1.0f); // 範囲を少し狭める
        crashParticleSystem_->SetVelocity(6.0f); // 縦方向に広げるため初速を上げる (5.0f -> 6.0f)
        crashParticleSystem_->SetSpread(1.0f); // 横への広がりを少し抑える (2.0f -> 1.0f)
        crashParticleSystem_->Emit(120); // 範囲が狭くなった分、少し減らして密度を保つ
    }
}

void EnemyTackleEffects::Cancel() {
    StopTelegraph();
    waves_.clear();
    if (rushParticleSystem_) rushParticleSystem_->Clear();
    if (rushCoreParticleSystem_) rushCoreParticleSystem_->Clear();
    if (crashParticleSystem_) crashParticleSystem_->Clear();
}

void EnemyTackleEffects::Update(float deltaTime) {
    for (auto it = waves_.begin(); it != waves_.end();) {
        it->timer += deltaTime;
        
        float t = (std::min)(1.0f, it->timer / it->maxLife);
        
        if (it->isCrash) {
            float easeOut = 1.0f - static_cast<float>(std::pow(1.0f - t, 3));
            float currentScale = Lerp(kCrashWaveStartScale, kCrashWaveEndScale, easeOut);
            it->transform.scale.x = currentScale;
            it->transform.scale.z = currentScale;
            it->transform.scale.y = Lerp(1.0f, 0.01f, t); // 高さを徐々に潰していく
            
            it->color.w = Lerp(kCrashWaveStartAlpha, 0.0f, t * t);

            // 当たり判定のスケール（Planeの広がり）に合わせて爆発パーティクルを発生させる
            if (crashParticleSystem_) {
                // currentScaleは全幅なので、半径はその半分。
                // RingではなくHemisphere（半球）エミッターを使うことで、横だけでなく縦（ドーム状）にも爆発を広げる
                crashParticleSystem_->SetHemisphereEmitter(it->transform.translate, currentScale * 0.5f, 0, 1.0f);
                crashParticleSystem_->SetVelocity(2.5f); // 縦方向に広げるため少し上げる (1.5f -> 2.5f)
                crashParticleSystem_->Emit(30);          // スケールに応じた放出数に調整
            }
        } else {
            float easeOut = 1.0f - static_cast<float>(std::pow(1.0f - t, 2));
            float currentScale = Lerp(kRushWaveStartScale, kRushWaveEndScale, easeOut);
            it->transform.scale.x = currentScale;
            it->transform.scale.z = currentScale;
            
            it->color.w = Lerp(kRushWaveStartAlpha, 0.0f, t);
        }

        if (it->timer >= it->maxLife) {
            it = waves_.erase(it);
        } else {
            ++it;
        }
    }

    if (rushParticleSystem_) rushParticleSystem_->Update();
    if (rushCoreParticleSystem_) rushCoreParticleSystem_->Update();
    if (crashParticleSystem_) crashParticleSystem_->Update();
}

void EnemyTackleEffects::Draw(IrufemiEngine* engine) {
    if (!engine) return;

    if (rushCoreParticleSystem_) {
        rushCoreParticleSystem_->SyncBeforeDraw();
        rushCoreParticleSystem_->Draw();
    }
    if (rushParticleSystem_) {
        rushParticleSystem_->SyncBeforeDraw();
        rushParticleSystem_->Draw();
    }
    if (crashParticleSystem_) {
        crashParticleSystem_->SyncBeforeDraw();
        crashParticleSystem_->Draw();
    }
    
    DrawTelegraph(engine);
}

OBB EnemyTackleEffects::TackleWave::GetOBB() const {
    OBB obb;
    obb.center = transform.translate;
    // OBBはハーフサイズ（全幅の半分）。高さは適当に持たせる
    obb.size = { transform.scale.x * 0.5f, 2.0f, transform.scale.z * 0.5f }; 

    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(transform.rotate);
    obb.orientations[0] = { rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2] };
    obb.orientations[1] = { rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2] };
    obb.orientations[2] = { rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2] };

    return obb;
}

void EnemyTackleEffects::DrawDebug(Line3DRegion* lineRegion) {
    if (!lineRegion) return;

    auto addObbLines = [&](const OBB& obb) {
        if (obb.size.x == 0.0f && obb.size.y == 0.0f && obb.size.z == 0.0f) return;
        Vector3 corners[8];
        for (int i = 0; i < 8; ++i) {
            Vector3 offset = { 0, 0, 0 };
            offset = Math::Add(offset, Math::Multiply((i & 1) ? obb.size.x : -obb.size.x, obb.orientations[0]));
            offset = Math::Add(offset, Math::Multiply((i & 2) ? obb.size.y : -obb.size.y, obb.orientations[1]));
            offset = Math::Add(offset, Math::Multiply((i & 4) ? obb.size.z : -obb.size.z, obb.orientations[2]));
            corners[i] = Math::Add(obb.center, offset);
        }
        Vector4 color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 危険がわかりやすいように赤色
        lineRegion->AddInstance(corners[0], corners[1], color);
        lineRegion->AddInstance(corners[1], corners[3], color);
        lineRegion->AddInstance(corners[3], corners[2], color);
        lineRegion->AddInstance(corners[2], corners[0], color);
        lineRegion->AddInstance(corners[4], corners[5], color);
        lineRegion->AddInstance(corners[5], corners[7], color);
        lineRegion->AddInstance(corners[7], corners[6], color);
        lineRegion->AddInstance(corners[6], corners[4], color);
        lineRegion->AddInstance(corners[0], corners[4], color);
        lineRegion->AddInstance(corners[1], corners[5], color);
        lineRegion->AddInstance(corners[2], corners[6], color);
        lineRegion->AddInstance(corners[3], corners[7], color);
    };

    for (const auto& wave : waves_) {
        addObbLines(wave.GetOBB());
    }
}
