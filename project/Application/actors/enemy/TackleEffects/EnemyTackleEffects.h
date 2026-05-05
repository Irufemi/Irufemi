#pragma once
#include "core/math/Transform.h"
#include "core/math/Vector4.h"
#include "core/math/geometry/OBB.h"
#include <memory>
#include <vector>
#include <list>

class Camera;
class ObjClass;

class EnemyTackleEffects {
public:
    struct TackleWave {
        Transform transform;
        float timer;
        float maxLife;
        Vector4 color;
        bool isCrash; // trueなら大爆発用、falseなら突進の砂煙用
        bool hasDealtDamage = false; // ダメージ判定済みフラグ

        OBB GetOBB() const;
    };

    void Initialize();
    void Update(float deltaTime);
    void Draw(class IrufemiEngine* engine);

    // 突進中に連続して呼ばれるエフェクト発生処理（背後や両脇に砂煙を残す）
    void FireRushWave(const Vector3& position);

    // 壁激突時に1度だけ呼ばれる大爆発エフェクト
    void FireCrashWave(const Vector3& position);

    // 当たり判定用に波のリストを取得
    std::list<TackleWave>& GetWaves() { return waves_; }

    void DrawDebug(class Line3DRegion* lineRegion);

private:
    Camera* camera_ = nullptr;
    
    // エフェクト描画用モデル（ひとまず既存の円環モデルを流用するためObjClassで平たく潰してリングにする）
    std::unique_ptr<ObjClass> waveObj_ = nullptr;

    std::list<TackleWave> waves_;

    // パラメータ
    const float kRushWaveLife = 0.2f; // 突進中の波の寿命
    const float kRushWaveStartScale = 10.0f;
    const float kRushWaveEndScale = 20.0f;
    const float kRushWaveStartAlpha = 0.9f;

    const float kCrashWaveLife = 2.0f; // 激突大爆発の波の寿命
    const float kCrashWaveStartScale = 20.0f;
    const float kCrashWaveEndScale = 80.0f;
    const float kCrashWaveStartAlpha = 0.9f;

    float Lerp(float start, float end, float t) const { return start + (end - start) * t; }
};
