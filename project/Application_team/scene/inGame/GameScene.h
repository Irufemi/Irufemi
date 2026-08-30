#pragma once

#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include <memory>

class IrufemiEngine;

/**
 * @class GameScene
 * @brief ゲーム本編を管理するクラス
 */
class GameScene : public BaseScene {
public: // メンバ関数(システム)
    ~GameScene() override;

    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    void DrawDebugTab() override;
};