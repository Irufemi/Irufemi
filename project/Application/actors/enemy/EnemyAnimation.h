#pragma once
#include "core/math/Vector3.h"

class Enemy;

class EnemyAnimation {
public:
    void Initialize(Enemy* enemy);
    void Update();

private:
    void UpdateIdle();       // 待機アニメーション
    void UpdateAttackBeam(); // ビーム攻撃前アニメーション

private:
    Enemy* enemy_ = nullptr;
    float timer_ = 0.0f;       // 全体共有タイマー（波の計算用）
    float attackTimer_ = 0.0f; // 攻撃フェーズ用タイマー

    // --- 調整用パラメータ：共通 ---
    float lerpSpeed_ = 0.1f;   // 通常の滑らかさ
    float gatherLerp_ = 0.15f; // 攻撃溜め時の素早さ

    // --- 調整用パラメータ：待機(Idle) ---
    float idleRotationSpeed_ = 0.005f; // 自転の速さ
    float idleWaveSpeed_ = 2.0f;       // 上下揺れの速さ
    float idleWaveHeight_ = 0.15f;     // 胴体の揺れ幅
    float idleHeadWaveHeight_ = 0.2f;  // 頭部の揺れ幅
    float idlePhaseOffset_ = 0.8f;     // 胴体ごとのズレ(位相)

    // --- 調整用パラメータ：ビーム攻撃(AttackBeam) ---
    float beamGatheringTime_ = 1.0f;   // パワー溜め(中央に寄る)時間
    float beamShakingTime_ = 1.5f;     // シェイクする時間
    float beamSinkDepth_ = -0.4f;      // 溜め時の沈み込み深さ
    float beamHeadGatherX_ = 1.0f;     // 溜め時の頭の寄せ位置(X)
    float beamShakeIntensityX_ = 0.15f; // シェイクの強さ(横)
    float beamShakeIntensityY_ = 0.05f; // シェイクの強さ(縦)
    float beamShakeSpeed_ = 80.0f;     // シェイクの速さ
};