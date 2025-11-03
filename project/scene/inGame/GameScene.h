#pragma once

#include "../IScene.h"

#include <memory>

#include "3D/TriangleClass.h"
#include "2D/Sprite.h"
#include "2D/Circle2D.h"
#include "2D/NumberText.h"
#include "2D/TimeDisplay.h"
#include "3D/SphereClass.h"
#include "3D/ObjClass.h"
#include "3D/ParticleClass.h"
#include "3D/CylinderClass.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "audio/Bgm.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "math/Vector2.h"
#include "math/shape/LinePrimitive.h"
#include "CollisionResult.h"

#include "Player.h"
#include "EnemyManager.h"
#include "../contents/CommonVisual.h"  // 追加

// 前方宣言
class IrufemiEngine;

class InputManager;

/// <summary>
/// ゲーム
/// </summary>
class GameScene : public IScene {
private: // 追加（床のワールド固定）
    // ワールド側の床（描画用）
    Segment2D groundWorld_[2]{};
    // 床帯の半径（ワールド）
    float groundRadiusWorld_ = 0.0f;
    // 床をワールドへ変換し終えたか
    bool groundConverted_ = false;

private: // メンバ関数(ゲーム部分)

    void GameSystem();
	
    void Reflection();
    void BulletRecovery();
    void EnemyProcess();

    // 追加: 地面シリンダーの更新（スクリーン→ワールド反映）
    void UpdateGroundObjects();

private:

    //反発係数
    static inline const float kCOR = 0.80f;
    static inline const float deltaTime = 1.0f / 60.0f;

    //敵の生成間隔の最小値、最大値
    const float kMinSpawnTimeFase1 = 1.0f;
    const float kMaxSpawnTimeFase1 = 5.0f;

    const float kMinSpawnTimeFase2 = 0.75f;
    const float kMaxSpawnTimeFase2 = 3.5f;

    const float kMinSpawnTimeFase3 = 0.4f;
    const float kMaxSpawnTimeFase3 = 1.5f;


private: // メンバ変数(ゲーム部分)

    std::unique_ptr<Player> player_;
    //エネミーマネージャー
    std::unique_ptr<EnemyManager> e_Manager_;

    //ゲームオーバーフラグ
    bool gameOver_;

    //地面
    Segment2D ground[2];
    CollisionResult p_result_[2];
    CollisionResult b_result_[2];

    // 追加: 地面描画用シリンダー
    std::unique_ptr<CylinderClass> groundObj_[2];
    // 追加: 地面の太さ [pixels]
    float groundThicknessPx_ = Visual::kGroundThicknessPx; // 共通化

    //コア
    Sphere circle_;
    int coreHp_;
    //弾とサークルの距離
    Vector2 vec_[10];
    float dis_[10];

    //===乱数生成器===///
    //生成エンジンの型
    using RNG_Engine = std::mt19937;
    //乱数エンジン
    RNG_Engine randomEngine_;
    //使う分布
    std::uniform_real_distribution<float> enemy_x_;

    //===敵のランダム生成===//
    //経過時間のカウント
    float ingameTimer_;
    //ゲーム時間のカウント
    float Timer_;
    //生成した乱数を入れる箱
    float time_;
    //乱数の分布(敵を生成する目標時間)
    std::uniform_real_distribution<float> spawnTime_[3];

    uint32_t killEnemyCount_;

    std::unique_ptr<Bgm> bgm = nullptr;

    std::unique_ptr<Se> se_enemy = nullptr;

    std::unique_ptr<Se> se_playerdamage = nullptr;

    std::unique_ptr<Se> se_playerstan = nullptr;

    std::unique_ptr<Se> se_playertoutch = nullptr;


    // --- private メンバ（適切な private セクションに追加） ---
    std::unique_ptr<NumberText> gameTimerText_;
    Vector2 gameTimerCenter_{ 400.0f, 32.0f }; // デフォルト表示中心（px）
    float  gameTimerScale_ = 1.0f;
    int    gameTimerDigits_ = 4;   // 固定 "XX.XX" 表示（小数点は別に描画する想定）
    float  gameTimeLimitSec_ = 60.0f; // 制限時間（秒）
    bool   showGameTimer_ = true;

    // ==== 追加: ゲームオーバー用メンバ ====

    // 結果表示用テキスト
    std::unique_ptr<NumberText> resultKillText_;
    std::unique_ptr<NumberText> resultTimeText_;
    // 結果表示用ドットスプライト
    std::unique_ptr<Sprite>    resultDotSprite_;
    // 表示桁数
    int    resultKillDigits_ = 4;     // 表示桁（倒した数）
    int    resultTimeDigits_ = 4;     // 表示桁（秒表示。例: "SScc" = 4桁で SS.cc）
    // スケール
    float  resultKillScale_  = 1.2f;
    float  resultTimeScale_  = 1.2f;
    // マージン
    float  resultKillMargin_ = 16.0f; // ラベルと数字の間隔(px)
    float  resultTimeMargin_ = 16.0f;
    // 表示フラグ
    bool   showResultNumbers_ = true;

    Vector2 resultKillPos_{ 0.0f, 0.0f };   // 倒した数表示の right-top（手動 or 自動）
    Vector2 resultTimePos_{ 0.0f, 0.0f };   // 生存時間表示の right-top（手動 or 自動）
    bool   resultKillManualPos_ = true;    // true のとき resultKillPos_ を固定（ImGuiで移動）
    bool   resultTimeManualPos_ = true;    // true のとき resultTimePos_ を固定（ImGuiで移動）

    std::unique_ptr<TimeDisplay> timeDisplayInGame_; // 追加
    std::unique_ptr<TimeDisplay> timeDisplayResult_; // 追加

private: // メンバ変数(ゲーム内描画物)

    // 文字(Bullet)
    std::unique_ptr<Sprite> text_bullet_ = nullptr;

    // 文字(HP)
    std::unique_ptr<Sprite> text_HP_ = nullptr;

    // 文字(/)
    std::unique_ptr<Sprite> text_slash_ = nullptr;

    // 文字(生き残れ)
    std::unique_ptr<Sprite> text_pleaseAlive_ = nullptr;

    // 文字(敵増える)
    std::unique_ptr<Sprite> text_addEnemy_ = nullptr;

    // 文字(倒した敵の数)
    std::unique_ptr<Sprite> text_killEnemy_ = nullptr;

    // 文字(生き残った時間   )
    std::unique_ptr<Sprite> text_aliveTime_ = nullptr;

    // 文字(回転入力 )
    std::unique_ptr<Sprite> text_rotate_ = nullptr;

    // 背景の倍率ゾーン
    std::unique_ptr<Circle2D> zoneCircles_[3];

    bool resultPhase_;

private: // メンバ変数(システム)

    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;

    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    std::unique_ptr<PointLightClass> pointLight_ = nullptr;

    std::unique_ptr<SpotLightClass> spotLight_ = nullptr;

    int loadTexture = false;

    bool debugMode = false;

    // ポインタ参照

    // エンジン
    IrufemiEngine* engine_ = nullptr;

    // ==== 追加: 弾数UI ====
    std::unique_ptr<NumberText> bulletNowText_;
    std::unique_ptr<NumberText> bulletMaxText_;
    Vector2 bulletUiLeftTop_{ 400.0f, 720.0f }; // 全体の左上
    float  bulletUiSlashGap_ = 2.0f;        // スラッシュ予定位置の隙間（後で'/'を入れるための余白）
    float  bulletUiBlockGap_ = 10.0f;         // ブロック間の余白
    int    bulletUiMaxValue_ = 10;           // 表示する最大値（固定表示「10」）

    // 追加：中央回収用の2D円（Circle2D）
    std::unique_ptr<Circle2D> coreCircle_ = nullptr;

    // === ゲームオーバー拡大パネル ===
    std::unique_ptr<Sprite> gameOverPlate_ = nullptr;
    bool   gameOverAnimActive_ = false;
    // 追加: 一度だけ発火させるためのフラグ
    bool   gameOverAnimPlayed_ = false;
    float  gameOverAnimTime_   = 0.0f;
    float  gameOverAnimDuration_ = 0.6f; // 秒
    Vector2 gameOverTargetSize_{ 0.0f, 0.0f };

    // ==== カウントダウン表示 ====
    std::unique_ptr<NumberText> countdownText_;
    bool countdownActive_ = false;            // カウント中フラグ
    float countdownTime_ = 0.0f;              // 残り時間（秒）
    int   countdownStartSeconds_ = 3;         // 開始秒（デフォルト3）
    Vector2 countdownCenter_{ 0.0f, 0.0f };   // 表示中心（画面中心を初期化時に設定）
    int   countdownDisplayDigits_ = 1;        // 現在の桁数（内部管理）

    // --- HP アイコン（Sprite: heart_gloss.png を横並び表示） ---
    static inline const int kMaxHpIcons = 10;
    std::unique_ptr<Sprite> hpIconSprites_[kMaxHpIcons];
    Vector2 hpIconsPos_{ 70.0f, 740.0f };     // 第1アイコンの中心（px）
    float   hpIconSpacing_ = 8.0f;            // アイコン間隔（px）
    float   hpIconScreenRadius_ = 12.0f;      // 半径（px）→ サイズは直径 2R を使用
    bool    hpIconsVisible_ = true;

public: // メンバ関数

    // デストラクタ
    ~GameScene() {}

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
};