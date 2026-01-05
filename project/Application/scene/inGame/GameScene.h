#pragma once

#include "scene/IScene.h"

#include <memory>
#include <vector> // vector をインクルード

#include "3D/TriangleClass.h"
#include "2D/Sprite.h"
#include "2D/Circle2D.h"
#include "2D/NumberText.h"
#include "2D/TimeDisplay.h"
#include "3D/SphereClass.h"
#include "3D/ObjClass.h"
#include "3D/Region.h"
#include "3D/particle/ParticleSystem.h"
#include "3D/CylinderClass.h"
#include "audio/Bgm.h"

#include "camera/CameraController.h"

#include "contents/player/Player.h"
#include "contents/skydome/Skydome.h"
#include "contents/MapChipField.h"
#include "contents/Effect/Fade.h"
#include "contents/enemy/IEnemy.h" // IEnemy をインクルード

// 前方宣言
class IrufemiEngine;
class InputManager;
class Camera;
class DebugCamera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;


/// <summary>
/// ゲーム
/// </summary>
class GameScene : public IScene {
private: // 関数

    void GenerateBlocks();

    // 敵の生成
    void GenerateEnemies();
    // 衝突判定
    void CheckAllCollisions();

private: // 変数(ゲームの歯車)

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

private: // メンバ変数(UI/HUD)
    // HP
    std::unique_ptr<Sprite> text_HP_ = nullptr;
    // HPBar
    // out
    std::unique_ptr<Sprite> hpBar_out_ = nullptr;
    // in
    std::unique_ptr<Sprite> hpBar_in_ = nullptr;
    float hpBarOriginalWidth_ = 0.0f;
    // ポーズ表示用
    std::unique_ptr<Sprite> pauseSprite_ = nullptr;
    // ポーズメニューUI
    std::unique_ptr<Sprite> pauseTitleText_ = nullptr;
    std::unique_ptr<Sprite> pauseReturnToGameText_ = nullptr;
    std::unique_ptr<Sprite> pauseReturnToTitleText_ = nullptr;

private: // ポーズメニューの状態
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

private: // 画面演出
    // フェード
    std::unique_ptr<Fade> fade_ = nullptr;

private: // メンバ変数(システム)

    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;

    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;

    std::unique_ptr<PointLight> pointLight_ = nullptr;

    std::unique_ptr<SpotLight> spotLight_ = nullptr;

    int loadTexture = false;

    bool debugMode = false;

    // ポインタ参照

    // エンジン
    IrufemiEngine* engine_ = nullptr;

public: // メンバ関数

    // デストラクタ
    ~GameScene();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(IrufemiEngine* engine) override;

    /// <summary>
    /// 更新
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画
    /// </summary>
    void Draw() override;

    /// <summary>
    /// ポーズ中の更新
    /// </summary>
    void PauseUpdate() override;

    /// <summary>
    /// ポーズ中の描画
    /// </summary>
    void PauseDraw() override;
};