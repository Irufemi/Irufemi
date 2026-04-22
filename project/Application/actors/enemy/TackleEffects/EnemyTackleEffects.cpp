#include "EnemyTackleEffects.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Irufemi.h" 
#include "Core/Math/Math.h"
#include <cmath>
#include <algorithm>

void EnemyTackleEffects::Initialize(Camera* camera) {
    camera_ = camera;
    waveObj_ = std::make_unique<ObjClass>();
    waveObj_->Initialize(camera_, "sample/block.obj");
}

void EnemyTackleEffects::FireRushWave(const Vector3& position) {
    TackleWave wave;
    wave.transform.translate = position;
    wave.transform.translate.y = position.y - 1.0f; // 地面付近
    wave.transform.rotate = { 0, 0, 0 };
    wave.transform.scale = { kRushWaveStartScale, 0.01f, kRushWaveStartScale };
    
    wave.timer = 0.0f;
    wave.maxLife = kRushWaveLife;
    wave.isCrash = false;
    wave.color = { 0.8f, 0.7f, 0.5f, kRushWaveStartAlpha }; // 砂煙っぽい色

    waves_.push_back(wave);
}

void EnemyTackleEffects::FireCrashWave(const Vector3& position) {
    TackleWave wave;
    wave.transform.translate = position;
    wave.transform.translate.y = position.y - 1.0f; 
    
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
    engine->ApplyPSO();

    for (auto& wave : waves_) {
        waveObj_->SetTransform(wave.transform);
        waveObj_->SetColor(wave.color);
        waveObj_->Update();
        waveObj_->Draw();
    }
}
