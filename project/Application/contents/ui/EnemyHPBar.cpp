#include "EnemyHPBar.h"

#include "Irufemi.h"
#include "actors/enemy/Enemy.h"
#include "actors/enemy/EnemyParameters.h"
#include "Engine/Graphics/Camera/Camera.h"

#include <algorithm>
#include <cmath>

namespace {
// --- レイアウト定数 ---
constexpr float kBarMarginTop = 20.0f;   ///< 画面上端からの余白
constexpr float kBarHeight = 20.0f;      ///< バーの高さ
constexpr float kBarWidthRatio = 0.45f;  ///< 画面幅に対するバーの割合
constexpr float kFramePadding = 3.0f;    ///< 枠線の余白

// --- アニメーション定数 ---
constexpr float kSmoothSpeed = 2.5f; ///< HP減少アニメーション速度

// --- HP色定義（線形補間用） ---
// hpRatio > 0.5 : 緑→黄
// hpRatio <= 0.5 : 黄→赤
constexpr float kColorGreenR = 0.15f, kColorGreenG = 0.85f, kColorGreenB = 0.25f;
constexpr float kColorYellowR = 0.95f, kColorYellowG = 0.85f, kColorYellowB = 0.15f;
constexpr float kColorRedR = 0.90f, kColorRedG = 0.15f, kColorRedB = 0.15f;

// --- 背景・枠色 ---
constexpr float kBgR = 0.08f, kBgG = 0.08f, kBgB = 0.08f, kBgA = 0.75f;
constexpr float kFrameR = 0.55f, kFrameG = 0.55f, kFrameB = 0.55f, kFrameA = 0.90f;
} // namespace

void EnemyHPBar::Initialize(int screenWidth, int screenHeight) {
    // バーのレイアウトを算出
    barMaxWidth_ = static_cast<float>(screenWidth) * kBarWidthRatio;
    barHeight_ = kBarHeight;
    barX_ = (static_cast<float>(screenWidth) - barMaxWidth_) * 0.5f; // 中央揃え
    barY_ = kBarMarginTop;

    // 最大HPの算出
    auto *params = EnemyParameters::GetInstance();
    maxHP_ = params->GetBodyHP() * 3 + params->GetHeadLeftHP() +
             params->GetHeadMidHP() + params->GetHeadRightHP();

    // --- 枠線スプライト（一番下のレイヤー） ---
    barFrame_ = std::make_unique<Sprite>();
    barFrame_->Initialize("resources/whiteTexture.png");
    barFrame_->SetSize(barMaxWidth_ + kFramePadding * 2.0f,
                       barHeight_ + kFramePadding * 2.0f);
    barFrame_->SetPosition(barX_ - kFramePadding, barY_ - kFramePadding);
    barFrame_->SetColor(
        Vector4{kFrameR, kFrameG, kFrameB, kFrameA});
    barFrame_->Update();

    // --- 背景スプライト ---
    barBg_ = std::make_unique<Sprite>();
    barBg_->Initialize("resources/whiteTexture.png");
    barBg_->SetSize(barMaxWidth_, barHeight_);
    barBg_->SetPosition(barX_, barY_);
    barBg_->SetColor(Vector4{kBgR, kBgG, kBgB, kBgA});
    barBg_->Update();

    // --- HP充填スプライト ---
    barFill_ = std::make_unique<Sprite>();
    barFill_->Initialize("resources/whiteTexture.png");
    barFill_->SetSize(barMaxWidth_, barHeight_);
    barFill_->SetPosition(barX_, barY_);
    barFill_->SetColor(
        Vector4{kColorGreenR, kColorGreenG, kColorGreenB, 1.0f});
    barFill_->Update();

    currentRatio_ = 1.0f;
    displayRatio_ = 1.0f;
}

void EnemyHPBar::Update(const Enemy *enemy) {
    if (!enemy) {
        currentRatio_ = 0.0f;
    } else {
        // 各部位のHPを合算
        auto clamp0 = [](int hp) { return hp < 0 ? 0 : hp; };
        int currentHP = 0;
        for (int i = 0; i < 3; ++i) {
            auto *body = const_cast<Enemy *>(enemy)->GetBody(i);
            if (body)
                currentHP += clamp0(body->GetHP());
        }
        {
            auto *hl = const_cast<Enemy *>(enemy)->GetHeadLeft();
            if (hl) currentHP += clamp0(hl->GetHP());
        }
        {
            auto *hm = const_cast<Enemy *>(enemy)->GetHeadMid();
            if (hm) currentHP += clamp0(hm->GetHP());
        }
        {
            auto *hr = const_cast<Enemy *>(enemy)->GetHeadRight();
            if (hr) currentHP += clamp0(hr->GetHP());
        }
        currentRatio_ =
            (maxHP_ > 0)
                ? static_cast<float>(currentHP) / static_cast<float>(maxHP_)
                : 0.0f;
    }

    // 表示用の割合をスムーズに近づける
    float dt = 1.0f / 60.0f; // 固定フレーム想定
    if (displayRatio_ > currentRatio_) {
        displayRatio_ -= kSmoothSpeed * dt;
        if (displayRatio_ < currentRatio_)
            displayRatio_ = currentRatio_;
    } else {
        displayRatio_ = currentRatio_;
    }

    displayRatio_ = (std::max)(0.0f, (std::min)(1.0f, displayRatio_));

    // バー幅の更新
    float fillWidth = barMaxWidth_ * displayRatio_;
    if (fillWidth < 1.0f && displayRatio_ > 0.0f)
        fillWidth = 1.0f; // 最低1px

    barFill_->SetSize(fillWidth, barHeight_);
    barFill_->SetPosition(barX_, barY_);

    // 色の更新
    UpdateBarColor(displayRatio_);

    // 行列更新
    barFrame_->Update();
    barBg_->Update();
    barFill_->Update();
}

void EnemyHPBar::Draw() {
    // 描画順: 枠 → 背景 → 充填
    if (barFrame_)
        barFrame_->Draw();
    if (barBg_)
        barBg_->Draw();
    if (barFill_)
        barFill_->Draw();
}

void EnemyHPBar::UpdateBarColor(float hpRatio) {
    float r, g, b;

    if (hpRatio > 0.5f) {
        // 緑→黄
        float t = (hpRatio - 0.5f) * 2.0f; // 1.0(緑) → 0.0(黄)
        r = kColorYellowR + (kColorGreenR - kColorYellowR) * t;
        g = kColorYellowG + (kColorGreenG - kColorYellowG) * t;
        b = kColorYellowB + (kColorGreenB - kColorYellowB) * t;
    } else {
        // 黄→赤
        float t = hpRatio * 2.0f; // 0.5(黄) → 0.0(赤)
        r = kColorRedR + (kColorYellowR - kColorRedR) * t;
        g = kColorRedG + (kColorYellowG - kColorRedG) * t;
        b = kColorRedB + (kColorYellowB - kColorRedB) * t;
    }

    barFill_->SetColor(Vector4{r, g, b, 1.0f});
}
