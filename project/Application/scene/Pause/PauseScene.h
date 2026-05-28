#pragma once

#include "Framework/BaseScene.h"
#include "Framework/UISelectionGroup.h"
#include <memory>

class IrufemiEngine;
class Sprite;

/**
 * @class PauseScene
 * @brief ゲーム中に重ねて表示されるポーズ画面シーン
 */
class PauseScene : public BaseScene {
public:
    PauseScene();
    ~PauseScene() override;

    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

    // ポーズシーンは下のシーンの更新を止めるが、描画は止めない（透過して下のシーンが見える）
    bool IsUpdateBlocking() const override { return true; }
    bool IsDrawBlocking() const override { return false; }
    bool IsCursorVisible() const override { return true; } // メニュー操作用にカーソル表示

private:
    std::unique_ptr<Sprite> pauseBgDimmerSprite_ = nullptr; ///< 背景暗転（グレー）用
    std::unique_ptr<Sprite> pauseTitleSprite_ = nullptr;
    std::unique_ptr<Sprite> pauseBackGameSprite_ = nullptr;
    std::unique_ptr<Sprite> pauseTutorialSkipSprite_ = nullptr; ///< チュートリアルスキップ用
    std::unique_ptr<Sprite> pauseBackTitleSprite_ = nullptr;

    bool isTutorial_ = false; ///< 現在の裏のシーンがチュートリアルかどうか

    /// @brief ポーズメニューの選択コントローラー
    UISelectionGroup pauseMenuSelection_;
};
