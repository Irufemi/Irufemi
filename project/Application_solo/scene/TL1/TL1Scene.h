#pragma once

#include "Framework/BaseScene.h"
#include <string>

class IrufemiEngine;

/**
 * @brief ツール開発・検証用の専用シーン
 */
class TL1Scene : public BaseScene {
public: // メンバ関数
    TL1Scene() = default;
    ~TL1Scene() override = default;

    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    void DrawDebugTab() override;

private: // メンバ変数
    std::string promptText_ = "";
    std::string imagePath_ = "";
};
