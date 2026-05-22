#include "EnemyTackleEffects.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Irufemi.h" 
#include "Core/Math/Math.h"
#include <cmath>
#include <algorithm>

void EnemyTackleEffects::Initialize() {
    waveObj_ = std::make_unique<ObjClass>();
    waveObj_->Initialize("sample/block.obj");

    // 予告線（AOE）用
    telegraphObj_ = std::make_shared<PrimitiveObjects3DClass>();
    telegraphObj_->Initialize(PrimitiveType::Cube, "resources/whiteTexture.png");
    telegraphObj_->GetMaterial().enableLighting = false; // ライティング無効
    telegraphObj_->SetCastShadows(false);                // 影を落とさない
}

void EnemyTackleEffects::StartTelegraph(const Vector3& position, float rotateY, float length, float width) {
    isTelegraphActive_ = true;
    telegraphTransform_.scale = { width, 0.1f, length };
    // 長さの半分だけ前にずらすことで、ボスの足元から前方に伸びるようにする
    float halfLength = length * 0.5f;
    Vector3 forward = { std::sin(rotateY), 0.0f, std::cos(rotateY) };
    telegraphTransform_.translate = { 
        position.x + forward.x * halfLength, 
        position.y - 2.4f, // 足元（地面）に這わせる
        position.z + forward.z * halfLength 
    };
    telegraphTransform_.rotate = { 0.0f, rotateY, 0.0f };
    telegraphObj_->SetTransform(telegraphTransform_);
    telegraphObj_->SetColor({ 1.0f, 0.0f, 0.0f, 0.0f }); // 最初は透明
    telegraphObj_->Update();
}

void EnemyTackleEffects::UpdateTelegraph(const Vector3& position, float rotateY, float warningRatio) {
    if (!isTelegraphActive_ || !telegraphObj_) return;
    
    // 位置の追従（Aim中など位置・角度が変わる場合に対応）
    float length = telegraphTransform_.scale.z;
    float halfLength = length * 0.5f;
    Vector3 forward = { std::sin(rotateY), 0.0f, std::cos(rotateY) };
    telegraphTransform_.translate = { 
        position.x + forward.x * halfLength, 
        position.y - 2.4f, // 足元（地面）に這わせる
        position.z + forward.z * halfLength 
    };
    telegraphTransform_.rotate = { 0.0f, rotateY, 0.0f };

    // warningRatio(0.0〜1.0) に応じて点滅速度とアルファ値を変化
    // 徐々に赤くなり、最後は激しく明滅する
    float blinkSpeed = Lerp(5.0f, 30.0f, warningRatio); 
    float blink = (std::sin(warningRatio * blinkSpeed) + 1.0f) * 0.5f; // 0.0 ~ 1.0
    float baseAlpha = Lerp(0.2f, 0.8f, warningRatio);
    
    // 赤＋少し黄色を混ぜて危険色を強調
    telegraphObj_->SetColor({ 1.0f, Lerp(0.2f, 0.5f, blink), 0.0f, baseAlpha * blink });
    telegraphObj_->SetTransform(telegraphTransform_);
    telegraphObj_->Update();
}

void EnemyTackleEffects::StopTelegraph() {
    isTelegraphActive_ = false;
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
}

void EnemyTackleEffects::FireCrashWave(const Vector3& position) {
    TackleWave wave;
    wave.transform.translate = position;
    wave.transform.translate.y = position.y - 2.5f; 
    
    // 縦に広がるようにすることもできるが、まずは巨大なリングベースにする
    wave.transform.rotate = { 0, 0, 0 };
    wave.transform.scale = { kCrashWaveStartScale, 1.0f, kCrashWaveStartScale }; // 少し厚みを持たせる
    
    wave.timer = 0.0f;
    wave.maxLife = kCrashWaveLife;
    wave.isCrash = true;
    wave.color = { 1.0f, 0.4f, 0.1f, kCrashWaveStartAlpha }; // 激しい爆発の色（オレンジ）

    waves_.push_back(wave);
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
}

void EnemyTackleEffects::Draw(IrufemiEngine* engine) {
    if (!engine) return;

    for (auto& wave : waves_) {
        waveObj_->SetTransform(wave.transform);
        waveObj_->SetColor(wave.color);
        waveObj_->Update();
        waveObj_->Draw();
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
