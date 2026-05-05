#pragma once

#include "Framework/BaseScene.h"
#include "Irufemi.h"

#include <memory>
#include <vector>

/**
 * @class SelectScene
 * @brief ステージ選択画面を管理するクラス
 *
 * プレイヤーがプレイするステージを選択し、決定に応じてゲームシーンへ遷移します。
 */
class SelectScene : public BaseScene {
public: // メンバ関数(システム)
    ~SelectScene() override;

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
    // 必要なゲームロジック用変数があればここに追加
};
