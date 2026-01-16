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

#include "audio/Bgm.h"
#include "audio/Se.h"
#include "3D/ObjClass.h"
#include "3D/Region.h"

#include "contents/effect/Fade.h"
#include "contents/player/Player.h"
#include "contents/enemy/normalEnemy/NormalEnemy.h"
#include "contents/enemy/shieldEnemy/ShieldEnemy.h"
#include "contents/skydome/Skydome.h"
#include "camera/CameraController.h"


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

    void GenerateBlocks();

    // 敵の生成
    void GenerateEnemies();
    // 衝突判定
    void CheckAllCollisions();

    // フェーズごとの更新処理
    void UpdateFadeIn();
    void UpdateCountdown();
    void UpdateGameplay();
    void UpdateFadeOut();

private: // メンバ変数(ゲーム)

    // カメラコントローラー
    std::unique_ptr<CameraController> cameraController_ = nullptr;

    // 天球
    std::unique_ptr<Skydome> skydome_ = nullptr;

    /// マップチップフィールド
    std::unique_ptr<MapChipField> mapChipField_ = nullptr;

    /// ブロック

    // ブロック群
    std::unique_ptr<class Region> blocks_ = nullptr;

    // ワールドトランスフォーム(ブロック)
    std::vector<std::vector<Transform*>> worldtransformBlocks_;

    /// 自キャラ

    // 自キャラ
    std::shared_ptr<Player> player_ = nullptr;

    // 3Dモデルデータ(自キャラ)
    std::unique_ptr<ObjClass> modelplayer_ = nullptr;

    // 3Dモデルデータ(自キャラの攻撃)
    std::unique_ptr<ObjClass> modelplayerAttack_ = nullptr;

    /// 敵キャラ
    std::vector<std::unique_ptr<IEnemy>> enemies_;
    
    // HP
    std::unique_ptr<Sprite> text_HP_ = nullptr;
    // HPBar
    // out
    std::unique_ptr<Sprite> hpBar_out_ = nullptr;
    // in
    std::unique_ptr<Sprite> hpBar_in_ = nullptr;
    float hpBarOriginalWidth_ = 0.0f;

    // ポーズメニューUI
    std::unique_ptr<Sprite> pauseTitleText_ = nullptr;
    std::unique_ptr<Sprite> pauseReturnToGameText_ = nullptr;
    std::unique_ptr<Sprite> pauseReturnToTitleText_ = nullptr;

    // カウントダウン(1)
    std::unique_ptr<Sprite> text_1_ = nullptr;
    // カウントダウン(2)
    std::unique_ptr<Sprite> text_2_ = nullptr;
    // カウントダウン(3)
    std::unique_ptr<Sprite> text_3_ = nullptr;
    // カウントダウン時のテキスト
    std::unique_ptr<Sprite> countdownText_killEnemy_ = nullptr;

    // 操作方法
    std::unique_ptr<Sprite> manual_ = nullptr;

    // bgm
    std::unique_ptr<Bgm> bgm_ = nullptr;
    // se(決定音)
    std::unique_ptr<Se> se_select_ = nullptr;

    // ゲーム進行の状態
    enum class Phase {
        FadeIn,
        Countdown,
        Gameplay,
        FadeOut,
    };
    Phase phase_ = Phase::FadeIn;
    float countdownTimer_ = 3.0f; // カウントダウンタイマー
    
    // 画面演出
    // フェード
    std::unique_ptr<Fade> fade_ = nullptr;

    // ポーズメニューの状態
    enum class PauseOption {
        ReturnToGame,
        ReturnToTitle,
    };
    PauseOption currentPauseOption_ = PauseOption::ReturnToGame;

    enum class PauseMenuState {
        Selecting,  // 項目選択中
        Confirming, // 項目決定演出中
    };
    PauseMenuState pauseMenuState_ = PauseMenuState::Selecting;

    float blinkTimer_ = 0.0f;         // 選択項目の明滅タイマー
    float confirmationTimer_ = 0.0f;  // 決定演出用のタイマー

private: // メンバ変数(システム)
    // エンジン
    IrufemiEngine* engine_ = nullptr;
    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;
    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    bool debugMode_ = false;
    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::unique_ptr<PointLight> pointLight_ = nullptr;
    std::unique_ptr<SpotLight> spotLight_ = nullptr;

    // ポーズ表示用
    std::unique_ptr<Sprite> pauseSprite_ = nullptr;
};