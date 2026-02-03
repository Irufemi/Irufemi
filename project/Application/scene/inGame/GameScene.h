#pragma once

#include "scene/IScene.h"

#include <memory>
#include <vector>

// 前方宣言
class IrufemiEngine;
class InputManager;
class Camera;
class DebugCamera;
class Sprite;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

#include "3D/ObjClass.h"
#include "math/Vector3.h"

#include "actors/player/Player.h"
#include "actors/healer/Healer.h"
#include "actors/healer/HealerActor.h"
#include "actors/enemy/Enemy.h"
#include "contents/wall/Wall.h"
#include "contents/UI/TimeDisplay.h"
// SE support
#include "audio/Se.h"

class EffectSystem;

/// <summary>
/// ゲーム
/// </summary>
class GameScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~GameScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    // ポーズ中の更新
    void PauseUpdate() override;
    // ポーズ中の描画
    void PauseDraw() override;
    // このシーンがポーズ可能かを返す
    bool IsPausable() const override { return true; }

private: // メンバ関数(内部ヘルパ)

    // 衝突判定
    void CollisionCheck();

    // フェーズの初期化
    void PhaseInitialize();

    // フェーズの更新
    void PhaseUpdate();

    // フェーズの変更
    void PhaseChange();

    // フェードインの初期化
    void FadeInInitialize();

    // フェードイン中の更新
    void FadeInUpdate();

    // ゲーム中の更新
    void GameInitialize();

    // ゲーム中の更新
    void GameUpdate();

    // フェードアウト中の更新
    void FadeOutInitialize();

    // フェードアウト中の更新
    void FadeOutUpdate();

    // 既存のゲームオブジェクトをすべて破棄・リセットする
    void ClearAllObjects();
    // モード分岐用
    void ModeInitialize();
    // 各モードの具体的生成ロジック
    void TutorialInitialize();
    void StandardInitialize();
  
    void StartCameraShake(Camera* cam, float duration, float magnitude);

#if defined(USE_IMGUI)
    // ImGuiでのデバッグ表示・操作
    void DebugImGui();
#endif

private: // メンバ変数(ゲーム進行)

    // フェーズ
    enum class Phase {
        FadeIn,
        Countdown,
        Game,
        FadeOut
    };

    // モード
    enum class GameMode {
        Tutorial,
        Standard,
    };

    // Phaseの初期化を行ったか
    bool isResetPhase_ = false;

    // Phase進行が完了したか
    bool isCompletePhase_ = false;

    // ゲームモード
    GameMode mode_;

    // フェーズ
    Phase phase_;

    // ゲームオーバーフラグ
    bool isGameOver_ = false;

    // チュートリアルからスタンダードへの移行フラグ
    bool isTransitioningToStandard_ = false;

    // フェード演出用
    std::unique_ptr<Sprite> fadeSprite_ = nullptr;
    float fadeTimer_ = 0.0f;
    static inline const float kFadeDuration_ = 1.0f; // 1秒でフェード

private: // メンバ変数(ゲーム)

    std::list<Wall*> walls_;
    static inline const int32_t kMaxWall_ = 16;

    std::list<Enemy*> enemies_;
    static inline const int32_t kMaxEnemy_ = 5;

    std::list<HealerActor*> healerActor_;
    static inline const int32_t kMaxHealerActor_ = 20;

    // Player(白血球)
    std::unique_ptr<Player> player_ = nullptr;

    // ヒーラー(血小板)
    std::unique_ptr<Healer> healer_ = nullptr;

    // タイマー(単位:秒)
    float timer_ = 0.0f;

    // 倒した敵の数
    int killScore_ = 0;

    // ゲームのプレイ時間(秒)
    int playTime_ = 60;

    std::unique_ptr<TimeDisplay> timeDisplay_;

  
    float cameraShakeTimer_ = 0.0f;
    float cameraShakeDuration_ = 0.0f;
    float cameraShakeMagnitude_ = 0.0f;
    Vector3 cameraShakeOriginalTranslate_{};

private: // メンバ変数(システム)
    // エンジン
    IrufemiEngine* engine_ = nullptr;
    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;
    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    // エフェクトシステム
    std::unique_ptr<EffectSystem> effectSystem_ = nullptr;

    bool debugMode_ = false;
    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;

    // ポーズ表示用
    std::unique_ptr<Sprite> pauseSprite_ = nullptr;

    // SE: プレイヤーと敵の接触時の効果音
    Se sePlayerHit_;
};