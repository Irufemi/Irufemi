#define NOMINMAX
#include "Fade.h"
#include "2D/Sprite.h"
#include "camera/Camera.h"
#include "engine/IrufemiEngine.h"
#include "function/Ease.h"

Fade::Fade() = default;
Fade::~Fade() = default;

IrufemiEngine* Fade::engine_ = nullptr;

void Fade::Initialize(Camera* camera) {
    sprite_ = std::make_unique<Sprite>();
    // 白テクスチャを使用し、任意の色を乗算して画面を覆います
    sprite_->Initialize(camera, "resources/whiteTexture.png");
    sprite_->SetPosition(0.0f, 0.0f);
    sprite_->SetAnchor(0.0f, 0.0f);
    // 画面サイズに合わせる
    sprite_->SetSize(static_cast<float>(engine_->GetClientWidth()), static_cast<float>(engine_->GetClientHeight()));
    // 初期状態では非表示
    sprite_->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
    sprite_->Update();
}

void Fade::Update() {

    if (state_ == FadeState::Idle) {
        return;
    }

    fadeTimer_ += 1.0f / 60.0f; // 固定フレームレートを想定

    float t = std::min(fadeTimer_ / fadeDuration_, 1.0f);
    float easedT = EaseOutSine(t);

    float alpha = 0.0f;
    if (state_ == FadeState::FadingIn) {
        // 徐々に透明になる (1 -> 0)
        alpha = Lerp(1.0f, 0.0f, easedT);
    } else if (state_ == FadeState::FadingOut) {
        // 徐々に不透明になる (0 -> 1)
        alpha = Lerp(0.0f, 1.0f, easedT);
    }

    sprite_->SetColor({ fadeColor_.x, fadeColor_.y, fadeColor_.z, alpha });
    sprite_->Update();

    if (fadeTimer_ >= fadeDuration_) {
        state_ = FadeState::Idle;
    }
}

void Fade::Draw() {
    if (state_ != FadeState::Idle) {
        // Sprite描画用のPSOを適用
        engine_->SetBlend(BlendMode::kBlendModeNormal);
        engine_->SetDepthWrite(PSOManager::DepthWrite::Disable);
        engine_->ApplySpritePSO();

        sprite_->Draw();
    }
}

void Fade::FadeIn(float duration, const Vector4& color) {
    state_ = FadeState::FadingIn;
    fadeDuration_ = duration;
    fadeTimer_ = 0.0f;
    fadeColor_ = color;
    // 開始時の色を設定
    sprite_->SetColor({ fadeColor_.x, fadeColor_.y, fadeColor_.z, 1.0f });
}

void Fade::FadeOut(float duration, const Vector4& color) {
    state_ = FadeState::FadingOut;
    fadeDuration_ = duration;
    fadeTimer_ = 0.0f;
    fadeColor_ = color;
    // 開始時の色を設定
    sprite_->SetColor({ fadeColor_.x, fadeColor_.y, fadeColor_.z, 0.0f });
}