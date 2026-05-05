#include "PlayerHPBar.h"
#include "actors/player/Player.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Core/Math/Vector4.h"
#include "Renderer/Object3D/Primitive/PlaneClass.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Engine/Graphics/Data/Material.h"
#include "Renderer/Object2D/Sprite/Sprite.h"
#include "IrufemiEngine.h"
#include <algorithm>
#include <cmath>

namespace {
// --- 3D空間でのレイアウト定数 ---
constexpr float kBarMaxWidth3D = 2.0f;     ///< バーの最大幅 (極小)
constexpr float kBarHeight3D = 0.1f;      ///< バーの高さ (極小)
constexpr float kFramePadding3D = 0.02f;   ///< 枠線の余白

// --- 2D空間でのレイアウト定数 ---
constexpr float kBarMaxWidth2D = 400.0f;
constexpr float kBarHeight2D = 24.0f;
constexpr float kFramePadding2D = 4.0f;

constexpr float kSmoothSpeed = 2.5f; ///< HP減少アニメーション速度

// --- HP色（Playerは青/水色ベースにする） ---
constexpr float kColorHighR = 0.15f, kColorHighG = 0.65f, kColorHighB = 0.95f; // 水色
constexpr float kColorLowR = 0.90f, kColorLowG = 0.15f, kColorLowB = 0.15f;    // 赤
constexpr float kBarAlpha = 0.5f; ///< バー本体の透明度 (0.0~1.0)

constexpr float kBgR = 0.08f, kBgG = 0.08f, kBgB = 0.08f, kBgA = 0.35f;          // 背景の透明度を少し下げる
constexpr float kFrameR = 0.75f, kFrameG = 0.75f, kFrameB = 0.75f, kFrameA = 0.45f; // 枠の透明度を下げる
} // namespace

void PlayerHPBar::Initialize(Camera* camera, IrufemiEngine* engine) {
    barMaxWidth3D_ = kBarMaxWidth3D;
    barHeight3D_ = kBarHeight3D;

    barMaxWidth2D_ = kBarMaxWidth2D;
    barHeight2D_ = kBarHeight2D;
    barX2D_ = 40.0f;
    barY2D_ = static_cast<float>(engine->GetClientHeight()) - 90.0f;

    // --- 3D ---
    barFrame_ = std::make_unique<PlaneClass>();
    barFrame_->Initialize(camera, "resources/whiteTexture.png");
    barFrame_->SetScale({barMaxWidth3D_ + kFramePadding3D * 2.0f, barHeight3D_ + kFramePadding3D * 2.0f, 1.0f});
    barFrame_->SetColor(Vector4{kFrameR, kFrameG, kFrameB, kFrameA});
    if (auto* mat = barFrame_->GetD3D12Resource()->GetMaterialData()) { mat->enableLighting = 0; mat->lightingMode = 0; }

    barBg_ = std::make_unique<PlaneClass>();
    barBg_->Initialize(camera, "resources/whiteTexture.png");
    barBg_->SetScale({barMaxWidth3D_, barHeight3D_, 1.0f});
    barBg_->SetColor(Vector4{kBgR, kBgG, kBgB, kBgA});
    if (auto* mat = barBg_->GetD3D12Resource()->GetMaterialData()) { mat->enableLighting = 0; mat->lightingMode = 0; }

    barFill_ = std::make_unique<PlaneClass>();
    barFill_->Initialize(camera, "resources/whiteTexture.png");
    barFill_->SetScale({barMaxWidth3D_, barHeight3D_, 1.0f});
    barFill_->SetColor(Vector4{kColorHighR, kColorHighG, kColorHighB, kBarAlpha});
    if (auto* mat = barFill_->GetD3D12Resource()->GetMaterialData()) { mat->enableLighting = 0; mat->lightingMode = 0; }

    // --- 2D ---
    spriteFrame_ = std::make_unique<Sprite>();
    spriteFrame_->Initialize(camera, "resources/whiteTexture.png");
    spriteFrame_->SetColor(Vector4{kFrameR, kFrameG, kFrameB, kFrameA});
    spriteFrame_->SetSize(barMaxWidth2D_ + kFramePadding2D * 2.0f, barHeight2D_ + kFramePadding2D * 2.0f);
    spriteFrame_->SetPositionTopLeft(barX2D_ - kFramePadding2D, barY2D_ - kFramePadding2D);

    spriteBg_ = std::make_unique<Sprite>();
    spriteBg_->Initialize(camera, "resources/whiteTexture.png");
    spriteBg_->SetColor(Vector4{kBgR, kBgG, kBgB, kBgA});
    spriteBg_->SetSize(barMaxWidth2D_, barHeight2D_);
    spriteBg_->SetPositionTopLeft(barX2D_, barY2D_);

    spriteFill_ = std::make_unique<Sprite>();
    spriteFill_->Initialize(camera, "resources/whiteTexture.png");
    spriteFill_->SetColor(Vector4{kColorHighR, kColorHighG, kColorHighB, kBarAlpha});
    spriteFill_->SetSize(barMaxWidth2D_, barHeight2D_);
    spriteFill_->SetPositionTopLeft(barX2D_, barY2D_);

    displayRatio_ = 1.0f;
}

void PlayerHPBar::Update(const Player *player, const Camera* camera, bool isFirstPerson) {
    if (!camera) return;

    float currentRatio = 0.0f;
    if (player) {
        int maxHP = player->GetMaxHp();
        int currentHP = player->GetHp();
        if (currentHP < 0) currentHP = 0;
        currentRatio = (maxHP > 0) ? (static_cast<float>(currentHP) / maxHP) : 0.0f;
    }

    float dt = 1.0f / 60.0f;
    if (displayRatio_ > currentRatio) {
        displayRatio_ -= kSmoothSpeed * dt;
        if (displayRatio_ < currentRatio) displayRatio_ = currentRatio;
    } else {
        displayRatio_ = currentRatio;
    }

    displayRatio_ = (std::max)(0.0f, (std::min)(1.0f, displayRatio_));

    UpdateBarColor(displayRatio_);

    // --- 3D 更新 (三人称のみ) ---
    if (!isFirstPerson) {
        float fillWidth3D = barMaxWidth3D_ * displayRatio_;
        if (fillWidth3D < 0.001f && displayRatio_ > 0.0f) fillWidth3D = 0.001f;

        Vector3 basePos = {0.0f, 0.0f, 0.0f};
        if (player) {
            basePos = player->GetTranslate();
            basePos.y += 1.5f;
        }
        
        Matrix4x4 billboardMat = Math::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, camera->GetRotate(), basePos);

        if (barFrame_) {
            barFrame_->SetScale({barMaxWidth3D_ + kFramePadding3D * 2.0f, barHeight3D_ + kFramePadding3D * 2.0f, 1.0f});
            Vector3 frameLocal = {0.0f, 0.0f, 0.02f};
            barFrame_->SetTranslate(Math::Transform(frameLocal, billboardMat));
            barFrame_->SetRotate(camera->GetRotate());
            barFrame_->Update();
        }
        if (barBg_) {
            barBg_->SetScale({barMaxWidth3D_, barHeight3D_, 1.0f});
            Vector3 bgLocal = {0.0f, 0.0f, 0.01f};
            barBg_->SetTranslate(Math::Transform(bgLocal, billboardMat));
            barBg_->SetRotate(camera->GetRotate());
            barBg_->Update();
        }
        if (barFill_) {
            barFill_->SetScale({fillWidth3D, barHeight3D_, 1.0f});
            float offsetX = -(barMaxWidth3D_ - fillWidth3D) * 0.5f;
            Vector3 fillLocal = {offsetX, 0.0f, 0.0f};
            barFill_->SetTranslate(Math::Transform(fillLocal, billboardMat));
            barFill_->SetRotate(camera->GetRotate());
            barFill_->Update();
        }
    }

    // --- 2D 更新 ---
    if (spriteFrame_) spriteFrame_->Update();
    if (spriteBg_) spriteBg_->Update();
    if (spriteFill_) {
        float fillWidth2D = barMaxWidth2D_ * displayRatio_;
        if (fillWidth2D < 0.001f && displayRatio_ > 0.0f) fillWidth2D = 0.001f;
        spriteFill_->SetSize(fillWidth2D, barHeight2D_);
        spriteFill_->Update();
    }
}

void PlayerHPBar::Draw3D(bool isUI) {
    if (barFrame_) barFrame_->Draw(isUI);
    if (barBg_) barBg_->Draw(isUI);
    if (barFill_) barFill_->Draw(isUI);
}

void PlayerHPBar::Draw2D() {
    if (spriteFrame_) spriteFrame_->Draw();
    if (spriteBg_) spriteBg_->Draw();
    if (spriteFill_) spriteFill_->Draw();
}

void PlayerHPBar::UpdateBarColor(float hpRatio) {
    float r, g, b;

    if (hpRatio > 0.3f) {
        r = kColorHighR; g = kColorHighG; b = kColorHighB;
    } else {
        float t = hpRatio / 0.3f;
        r = kColorLowR + (kColorHighR - kColorLowR) * t;
        g = kColorLowG + (kColorHighG - kColorLowG) * t;
        b = kColorLowB + (kColorHighB - kColorLowB) * t;
    }
    if (barFill_) barFill_->SetColor(Vector4{r, g, b, kBarAlpha});
    if (spriteFill_) spriteFill_->SetColor(Vector4{r, g, b, kBarAlpha});
}
