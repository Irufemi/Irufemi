#include "PlayerHPBar.h"
#include "actors/player/Player.h"
#include "camera/Camera.h"
#include "Engine/Core/Math/Vector4.h"
#include "Renderer/Object3D/Primitive/PlaneClass.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Engine/Graphics/Data/Material.h"
#include <algorithm>
#include <cmath>

namespace {
// --- 3D空間でのレイアウト定数 ---
constexpr float kBarMaxWidth = 2.0f;     ///< バーの最大幅 (極小)
constexpr float kBarHeight = 0.1f;      ///< バーの高さ (極小)
constexpr float kBarPullIn = 0.0f;       ///< カメラ方向への引き寄せ限界 (今回は引き寄せずモデルと同じ奥行きにする)
constexpr float kFramePadding = 0.02f;   ///< 枠線の余白

constexpr float kSmoothSpeed = 2.5f; ///< HP減少アニメーション速度

// --- HP色（Playerは青/水色ベースにする） ---
constexpr float kColorHighR = 0.15f, kColorHighG = 0.65f, kColorHighB = 0.95f; // 水色
constexpr float kColorLowR = 0.90f, kColorLowG = 0.15f, kColorLowB = 0.15f;    // 赤
constexpr float kBarAlpha = 0.5f; ///< バー本体の透明度 (0.0~1.0)

constexpr float kBgR = 0.08f, kBgG = 0.08f, kBgB = 0.08f, kBgA = 0.35f;          // 背景の透明度を少し下げる
constexpr float kFrameR = 0.75f, kFrameG = 0.75f, kFrameB = 0.75f, kFrameA = 0.45f; // 枠の透明度を下げる
} // namespace

void PlayerHPBar::Initialize(Camera *camera) {
    barMaxWidth_ = kBarMaxWidth;
    barHeight_ = kBarHeight;

    barFrame_ = std::make_unique<PlaneClass>();
    barFrame_->Initialize(camera, "resources/whiteTexture.png");
    barFrame_->SetScale({barMaxWidth_ + kFramePadding * 2.0f, barHeight_ + kFramePadding * 2.0f, 1.0f});
    barFrame_->SetColor(Vector4{kFrameR, kFrameG, kFrameB, kFrameA});
    if (auto* mat = barFrame_->GetD3D12Resource()->GetMaterialData()) {
        mat->enableLighting = 0;
        mat->lightingMode = 0;
    }

    barBg_ = std::make_unique<PlaneClass>();
    barBg_->Initialize(camera, "resources/whiteTexture.png");
    barBg_->SetScale({barMaxWidth_, barHeight_, 1.0f});
    barBg_->SetColor(Vector4{kBgR, kBgG, kBgB, kBgA});
    if (auto* mat = barBg_->GetD3D12Resource()->GetMaterialData()) {
        mat->enableLighting = 0;
        mat->lightingMode = 0;
    }

    barFill_ = std::make_unique<PlaneClass>();
    barFill_->Initialize(camera, "resources/whiteTexture.png");
    barFill_->SetScale({barMaxWidth_, barHeight_, 1.0f});
    barFill_->SetColor(Vector4{kColorHighR, kColorHighG, kColorHighB, kBarAlpha});
    if (auto* mat = barFill_->GetD3D12Resource()->GetMaterialData()) {
        mat->enableLighting = 0;
        mat->lightingMode = 0;
    }

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
        if (displayRatio_ < currentRatio)
            displayRatio_ = currentRatio;
    } else {
        displayRatio_ = currentRatio;
    }

    displayRatio_ = (std::max)(0.0f, (std::min)(1.0f, displayRatio_));

    float fillWidth = barMaxWidth_ * displayRatio_;
    if (fillWidth < 0.001f && displayRatio_ > 0.0f) fillWidth = 0.001f;

    Vector3 basePos = {0.0f, 0.0f, 0.0f};
    if (isFirstPerson) {
        // 一人称視点：カメラの固定位置（画面右上付近）に配置
        Vector3 camPos = camera->GetTranslate();
        Vector3 camRot = camera->GetRotate();
        Matrix4x4 camMat = Math::MakeRotateXYZMatrix(camRot);

        Vector3 forward = { camMat.m[2][0], camMat.m[2][1], camMat.m[2][2] };
        Vector3 right = { camMat.m[0][0], camMat.m[0][1], camMat.m[0][2] };
        Vector3 up = { camMat.m[1][0], camMat.m[1][1], camMat.m[1][2] };

        // カメラから少し前方に離し、下中央付近に配置
        basePos = camPos + (forward * 2.0f) + (up * -0.7f);
    } else {
        // 三人称視点：プレイヤーの少し上（本来は見えない設定だが、データ更新用に計算）
        if (player) {
            basePos = player->GetTranslate();
            basePos.y += 1.5f;
        }
    }
    
    Matrix4x4 billboardMat = Math::MakeAffineMatrix({1.0f, 1.0f, 1.0f}, camera->GetRotate(), basePos);

    if (barFrame_) {
        barFrame_->SetScale({barMaxWidth_ + kFramePadding * 2.0f, barHeight_ + kFramePadding * 2.0f, 1.0f});
        Vector3 frameLocal = {0.0f, 0.0f, 0.02f};
        barFrame_->SetTranslate(Math::Transform(frameLocal, billboardMat));
        barFrame_->SetRotate(camera->GetRotate());
        barFrame_->Update();
    }

    if (barBg_) {
        barBg_->SetScale({barMaxWidth_, barHeight_, 1.0f});
        Vector3 bgLocal = {0.0f, 0.0f, 0.01f};
        barBg_->SetTranslate(Math::Transform(bgLocal, billboardMat));
        barBg_->SetRotate(camera->GetRotate());
        barBg_->Update();
    }

    if (barFill_) {
        barFill_->SetScale({fillWidth, barHeight_, 1.0f});
        float offsetX = -(barMaxWidth_ - fillWidth) * 0.5f;
        Vector3 fillLocal = {offsetX, 0.0f, 0.0f};
        barFill_->SetTranslate(Math::Transform(fillLocal, billboardMat));
        barFill_->SetRotate(camera->GetRotate());
        UpdateBarColor(displayRatio_);
        barFill_->Update();
    }
}

void PlayerHPBar::Draw() {
    if (barFrame_) barFrame_->Draw();
    if (barBg_) barBg_->Draw();
    if (barFill_) barFill_->Draw();
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
    barFill_->SetColor(Vector4{r, g, b, kBarAlpha});
}
