#pragma once

#include "Framework/BaseScene.h"
#include "Framework/GameObject.h"
#include <memory>

class IrufemiEngine;

/**
 * @class CG4Scene
 * @brief 評価課題（CG4）の検証・テスト用シーン
 */
class CG4Scene : public BaseScene {
public: // メンバ関数(システム)
    ~CG4Scene() override;

    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    void DrawDebugTab() override;
};
