#define NOMINMAX
#include "TimeDisplay.h"
#include "engine/IrufemiEngine.h"
#include "camera/Camera.h"
#include "2D/NumberText.h"
#include "2D/Sprite.h"
#include "imgui.h"
#include <cstdio>
#include <algorithm>
#include <cmath>

TimeDisplay::~TimeDisplay() = default;

void TimeDisplay::Initialize(IrufemiEngine* engine, Camera* camera, const char* name,
    const std::string& fontTex, float cellW, float cellH, int digits, Vector2 center, float scale) {

    engine_ = engine;
    camera_ = camera;
    name_ = name ? name : "TimeDisplay";
    digits_ = digits;
    scale_ = scale;
    center_ = center;

    numText_ = std::make_unique<NumberText>();
    numText_->Initialize(camera_, fontTex, cellW, cellH, static_cast<size_t>(digits_));
    numText_->SetTracking(0.0f);
    numText_->SetScale(scale_);

    // 内部ドット（クラスで保持）。初期サイズはアセット 19x38 に numText_->GetScale() と dotScale_ を乗算
    dotSprite_ = std::make_unique<Sprite>();
    dotSprite_->Initialize(camera_, "resources/texture/gameText_period.png");
    dotSprite_->SetAnchor(0.5f, 0.5f);
    dotSprite_->SetSize(19.0f * numText_->GetScale() * dotScale_, 38.0f * numText_->GetScale() * dotScale_);

    // 初期オフセットは自動計算に任せる
    dotOffsetFromCenter_ = Vector2{ 0.0f, 0.0f };
    dotOffsetManual_ = false;
    dotScaleManual_ = false;
    dotYOffsetRatio_ = 0.0f;
}

void TimeDisplay::Update(float timer) {
    lastTimer_ = timer;
    if (dotSprite_) {
        std::string dbg = name_ + "_dot";
        dotSprite_->Update(true, dbg.c_str());
    }
}

void TimeDisplay::Debug() {
    std::string win = std::string("TimeDisplay - ") + name_;
    if (!ImGui::Begin(win.c_str())) { ImGui::End(); return; }

    ImGui::Checkbox("Visible", &visible_);
    ImGui::DragFloat2("Center (px)", &center_.x, 1.0f);

    if (ImGui::DragFloat("Scale", &scale_, 0.01f, 0.1f, 10.0f)) {
        if (numText_) numText_->SetScale(scale_);
        if (dotSprite_) dotSprite_->SetSize(19.0f * numText_->GetScale() * dotScale_, 38.0f * numText_->GetScale() * dotScale_);
    }

    int d = digits_;
    if (ImGui::SliderInt("Digits", &d, 2, 6)) {
        digits_ = d;
        if (numText_) numText_->SetMaxDigits(static_cast<size_t>(digits_));
    }
    ImGui::Checkbox("Manual pos", &manualPos_);

    ImGui::Separator();

    // --- 自動オフセットの計算（Debug 内でも利用できるように算出） ---
    const size_t pad = static_cast<size_t>(std::max(2, digits_));
    const float digitsW = numText_->GetWidthForDigits(pad);
    const float scaledCellW = numText_->GetCellW() * numText_->GetScale();
    const float scaledCellH = numText_->GetCellH() * numText_->GetScale();
    const float scaledTracking = numText_->GetTracking() * numText_->GetScale();

    const float centerX = center_.x;
    const float centerY = center_.y;
    const float rightTopX = centerX + digitsW * 0.5f;
    const float topY = centerY - (scaledCellH * 0.5f);
    const float leftX = rightTopX - digitsW;
    const int integerCount = std::max(0, static_cast<int>(pad) - 2);
    const float integerBlockWidth = static_cast<float>(integerCount) * scaledCellW
        + static_cast<float>(std::max(0, integerCount - 1)) * scaledTracking;
    const float autoDotCenterX = leftX + integerBlockWidth + (scaledTracking * 0.5f);
    const float autoDotCenterY = topY + (scaledCellH * (0.5f + dotYOffsetRatio_));
    const Vector2 autoOffset = Vector2{ autoDotCenterX - centerX, autoDotCenterY - centerY };

    // ドット位置の手動/自動切替とリセット
    ImGui::Checkbox("Manual dot offset", &dotOffsetManual_);
    if (dotOffsetManual_) {
        ImGui::DragFloat2("Dot Offset (px)", &dotOffsetFromCenter_.x, 0.5f);
        if (ImGui::Button("Reset to auto offset")) {
            dotOffsetFromCenter_ = autoOffset;
        }
    } else {
        ImGui::Text("Auto dot offset: (%.1f, %.1f)", autoOffset.x, autoOffset.y);
        if (ImGui::Button("Apply auto offset now")) {
            dotOffsetFromCenter_ = autoOffset;
        }
    }

    ImGui::Separator();

    // ドットのスケール倍率を操作
    ImGui::SliderFloat("Dot scale multiplier", &dotScale_, 0.25f, 3.0f, "%.2f");
    if (ImGui::Button("Reset dot scale")) {
        dotScale_ = 1.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Sync dot size")) {
        if (dotSprite_) dotSprite_->SetSize(19.0f * numText_->GetScale() * dotScale_, 38.0f * numText_->GetScale() * dotScale_);
    }
    ImGui::Text("Effective dot size: %.1f x %.1f px", 19.0f * numText_->GetScale() * dotScale_, 38.0f * numText_->GetScale() * dotScale_);

    ImGui::Separator();
    // Y オフセット比率（セル高さ基準）も編集可能
    ImGui::SliderFloat("Dot Y offset ratio (rel cellH)", &dotYOffsetRatio_, -0.2f, 0.2f, "%.3f");

    ImGui::End();
}

std::string TimeDisplay::FormatCentis(int centis, int pad) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "%0*d", pad, centis);
    return std::string(buf);
}

void TimeDisplay::Draw(float timer, bool remaining, float timeLimit) {
    if (!visible_ || !camera_ || !numText_) return;

    const size_t pad = static_cast<size_t>(std::max(2, digits_));
    float val = remaining ? std::max(0.0f, timeLimit - timer) : timer;
    const int centis = static_cast<int>(std::floor(val * 100.0f + 0.5f));
    const std::string s = FormatCentis(centis, static_cast<int>(pad));

    const float digitsW = numText_->GetWidthForDigits(pad);
    const float scaledCellW = numText_->GetCellW() * numText_->GetScale();
    const float scaledCellH = numText_->GetCellH() * numText_->GetScale();
    const float scaledTracking = numText_->GetTracking() * numText_->GetScale();

    const float centerX = center_.x;
    const float centerY = center_.y;

    const float rightTopX = centerX + digitsW * 0.5f;
    const float topY = centerY - (scaledCellH * 0.5f);

    numText_->SetPosRightTop(Vector2{ rightTopX, topY });
    numText_->DrawString(s);

    // ドット '.' の描画（内部保持）
    if (dotSprite_) {
        float dotCenterX = 0.0f;
        float dotCenterY = 0.0f;

        // アセット基準のサイズ（19x38）を数字スケール・倍率に合わせる
        const float dotW = 19.0f * numText_->GetScale() * dotScale_;
        const float dotH = 38.0f * numText_->GetScale() * dotScale_;

        if (dotOffsetManual_) {
            // 手動指定 (center 基準のオフセット px)
            dotCenterX = centerX + dotOffsetFromCenter_.x;
            dotCenterY = centerY + dotOffsetFromCenter_.y;
        } else {
            // 自動計算：整数部の幅から X を決め、Y は cellH の中心 + 比率
            const float leftX = rightTopX - digitsW;
            const int integerCount = std::max(0, static_cast<int>(pad) - 2);
            const float integerBlockWidth = static_cast<float>(integerCount) * scaledCellW
                + static_cast<float>(std::max(0, integerCount - 1)) * scaledTracking;
            dotCenterX = leftX + integerBlockWidth + (scaledTracking * 0.5f);

            dotCenterY = topY + (scaledCellH * (0.5f + dotYOffsetRatio_));

            // 相対オフセットを保存（center 基準）
            dotOffsetFromCenter_.x = dotCenterX - centerX;
            dotOffsetFromCenter_.y = dotCenterY - centerY;
        }

        dotSprite_->SetSize(dotW, dotH);
        dotSprite_->SetAnchor(0.5f, 0.5f);
        dotSprite_->SetPosition(dotCenterX, dotCenterY);
        dotSprite_->Draw();
    }
}