#pragma once

#include "Framework/BaseScene.h"
#include "Framework/UISelectionGroup.h"
#include "Framework/UIAnimator.h"
#include "Renderer/Object2D/Sprite/Sprite.h"
#include <memory>
#include <vector>
#include "Resource/Audio/Bgm.h"

class IrufemiEngine;
class ObjClass;
class GPUParticleSystem;
#include "contents/skydome/Skydome.h"

/**
 * @class ResultScene
 * @brief ゲームの結果（クリア/ゲームオーバー）を表示するクラス
 *
 * ゲームの結果に応じてUIを表示し、入力に応じてステージ選択シーンへ戻ります。
 */
class ClearScene : public BaseScene {
public: // メンバ関数(システム)
    ~ClearScene() override;
    /**
     * @brief 初期化処理
     * @param engine IrufemiEngineのポインタ
     */
    void Initialize(IrufemiEngine* engine) override;
    /**
     * @brief 毎フレームの更新処理
     */
    void Update() override;
    /**
     * @brief 描画処理
     */
    void Draw() override;
    void DrawDebugTab() override;

private: // メンバ変数(ゲーム)

    // BGM
    std::unique_ptr<Bgm> bgm_ = nullptr;

    // 「Clear!!」文字
    std::unique_ptr<ObjClass> clearTextC_ = nullptr;
    std::unique_ptr<ObjClass> clearTextL_ = nullptr;
    std::unique_ptr<ObjClass> clearTextE_ = nullptr;
    std::unique_ptr<ObjClass> clearTextA_ = nullptr;
    std::unique_ptr<ObjClass> clearTextR_ = nullptr;
    std::unique_ptr<ObjClass> clearTextEx_ = nullptr;

    // 選択肢文字
    std::unique_ptr<ObjClass> objRetry_ = nullptr;
    std::unique_ptr<ObjClass> objBackToTitle_ = nullptr;

    // 祝祭パーティクル（紙吹雪と花火）
    std::unique_ptr<GPUParticleSystem> confettiParticles_ = nullptr;
    std::unique_ptr<GPUParticleSystem> fireworksParticles_ = nullptr;

    // 背景
    std::unique_ptr<Skydome> skydome_ = nullptr;

    // 演出状態管理
    float introTimer_ = 0.0f;
    int currentSlamIndex_ = -1; // 落とした文字の数
    bool isSlamming_ = true;
    bool isRainingConfetti_ = false; // 降らせる演出に移行したか
    float cameraAngle_ = 0.0f;
    float fireworksTimer_ = 0.0f;

private: // メンバ変数(システム)

    UISelectionGroup clearSelection_;
    UIAnimator clearTextAnimator_;

    // タイム表示用UI
    std::unique_ptr<Sprite> timeMinutes10_ = nullptr;
    std::unique_ptr<Sprite> timeMinutes1_ = nullptr;
    std::unique_ptr<Sprite> timeColon_ = nullptr;
    std::unique_ptr<Sprite> timeSeconds10_ = nullptr;
    std::unique_ptr<Sprite> timeSeconds1_ = nullptr;

    // 最高記録表示用UI
    std::unique_ptr<Sprite> bestMinutes10_ = nullptr;
    std::unique_ptr<Sprite> bestMinutes1_ = nullptr;
    std::unique_ptr<Sprite> bestColon_ = nullptr;
    std::unique_ptr<Sprite> bestSeconds10_ = nullptr;
    std::unique_ptr<Sprite> bestSeconds1_ = nullptr;

    // タイトル表示用UI
    std::unique_ptr<Sprite> thisRecordTitleSprite_ = nullptr;
    std::unique_ptr<Sprite> bestRecordTitleSprite_ = nullptr;

    // 最高記録保持用
    float bestTime_ = 0.0f;

    // タイム表示初期化完了フラグ
    bool isTimeSpritesInitialized_ = false;
};
