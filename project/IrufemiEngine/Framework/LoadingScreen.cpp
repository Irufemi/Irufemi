#include "LoadingScreen.h"
#include "../Engine/IrufemiEngine.h"
#include "../Renderer/Object2D/Sprite/Sprite.h"
#include "../Renderer/Object2D/Primitive/Circle2D.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "../Engine/Graphics/Pipeline/PSOManager.h"

LoadingScreen::LoadingScreen() = default;
LoadingScreen::~LoadingScreen() = default;

void LoadingScreen::Initialize(IrufemiEngine* engine) {
    if (!engine) return;

    camera_ = std::make_unique<Camera>();
    camera_->Initialize(engine->GetClientWidth(), engine->GetClientHeight());
    camera_->UpdateMatrix();

    nowLoadingText_ = std::make_unique<Sprite>();
    // 生成した「Now Loading」画像をセット
    nowLoadingText_->Initialize(camera_.get(), "resources/texture/load/now_loading.png");
    
    // 画像は黒背景に文字が含まれる
    // 描画時に加算合成(Add)を使うことで、黒を透過させる
    nowLoadingText_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

    float screenW = static_cast<float>(engine->GetClientWidth());
    float screenH = static_cast<float>(engine->GetClientHeight());
    
    // スプライトのサイズと位置を右下に合わせる
    // 画像は正方形(1:1)なので、縮尺がおかしくならないよう同サイズにする
    nowLoadingText_->SetSize(256.0f, 256.0f);
    nowLoadingText_->SetAnchor(1.0f, 0.5f); // 右端・縦中央アンカー
    nowLoadingText_->SetPosition(screenW - 80.0f, screenH - 45.0f); // ドットの高さと合わせる
    
    // "..." のドットを3つ作る
    for (int i = 0; i < 3; ++i) {
        auto dot = std::make_unique<Circle2D>();
        // Circle2Dは白テクスチャを内部的に使って丸を描く
        dot->Initialize(camera_.get(), "resources/whiteTexture.png", 16); 
        dot->SetUseTexture(false); 
        dot->SetColor({1.0f, 1.0f, 1.0f, 1.0f}); // 文字のNeon Cyanに合わせた色にする
        
        // 中心座標をセット: nowLoadingText_ のさらに右側に等間隔で配置
        float baseX = screenW - 65.0f; 
        float baseY = screenH - 45.0f; 
        dot->SetCenter({baseX + i * 20.0f, baseY, 0.0f});
        dot->SetRadius(4.0f);
        
        // 最前面UIとして登録
        dot->SetTopMost(true);

        dots_.push_back(std::move(dot));
    }

    // 最前面UIとして登録
    nowLoadingText_->SetTopMost(true);
}

void LoadingScreen::Update(float deltaTime) {
    animationTimer_ += deltaTime;
    // 0.5秒ごとにドットが1個増え、0 -> 1 -> 2 -> 3 -> 0 のループになる
    const float kDotInterval = 0.5f;
    if (animationTimer_ > kDotInterval) {
        animationTimer_ -= kDotInterval;
        dotCount_ = (dotCount_ + 1) % 4; 
    }

    camera_->Update();
    if (nowLoadingText_) {
        nowLoadingText_->Update();
    }
    for (auto& dot : dots_) {
        dot->Update();
    }
}

void LoadingScreen::Draw(IrufemiEngine* engine) {
    if (!engine) return;

    if (nowLoadingText_) {
        nowLoadingText_->Draw();
    }
    
    for (int i = 0; i < dotCount_; ++i) {
        if (i < dots_.size()) {
            dots_[i]->Draw();
        }
    }
}
