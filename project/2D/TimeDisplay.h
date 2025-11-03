#pragma once

#include <memory>
#include <string>
#include "math/Vector2.h"

class IrufemiEngine;
class Camera;
class NumberText;
class Sprite;

class TimeDisplay {
public:
    TimeDisplay() = default;
    ~TimeDisplay();

    // name: デバッグ用の識別名
    void Initialize(IrufemiEngine* engine, Camera* camera, const char* name,
        const std::string& fontTex = "resources/text_num.png",
        float cellW = 32.0f, float cellH = 64.0f,
        int digits = 4, Vector2 center = Vector2{ 640.0f, 32.0f },
        float scale = 1.0f);

    // timer: 経過秒。active=true のとき内部 sprite の Update を行う
    void Update(float timer);

    // ImGui デバッグ表示（呼出可）
    void Debug();

    // 描画
    // remaining==true のときは timeLimit - timer を表示（残り時間）
    // remaining==false のときは timer（経過時間）を表示
    void Draw(float timer, bool remaining, float timeLimit = 60.0f);

    void SetVisible(bool v) { visible_ = v; }

    // ドットの相対オフセット（center を基準としたピクセル単位）
    void SetDotOffsetFromCenter(const Vector2& offset) { dotOffsetFromCenter_ = offset; dotOffsetManual_ = true; }
    Vector2 GetDotOffsetFromCenter() const { return dotOffsetFromCenter_; }
    void SetDotOffsetManualEnabled(bool en) { dotOffsetManual_ = en; }
    bool IsDotOffsetManualEnabled() const { return dotOffsetManual_; }

    // ドットYの相対係数（セル高さに対する比率、0.0=縦センター）
    void SetDotYOffsetRatio(float r) { dotYOffsetRatio_ = r; }
    float GetDotYOffsetRatio() const { return dotYOffsetRatio_; }

    // ドットスケール（倍率。1.0 = アセット基準）
    void SetDotScale(float s) { dotScale_ = s; }
    float GetDotScale() const { return dotScale_; }
    void SetDotScaleManualEnabled(bool en) { dotScaleManual_ = en; }
    bool IsDotScaleManualEnabled() const { return dotScaleManual_; }

private:
    std::string FormatCentis(int centis, int pad) const;

private:
    std::string name_;
    IrufemiEngine* engine_ = nullptr;
    Camera* camera_ = nullptr;
    std::unique_ptr<NumberText> numText_;
    std::unique_ptr<Sprite> dotSprite_;            // 内部で保持するドットスプライト

    Vector2 center_{ 0.0f, 0.0f }; // 表示中心 (px)
    int digits_ = 4;
    float scale_ = 1.0f;
    float tracking_ = 0.0f;

    bool visible_ = true;
    bool manualPos_ = true;

    float lastTimer_ = 0.0f;

    // ドットの center_ に対するオフセット（ピクセル）。自動計算もしくは手動指定。
    Vector2 dotOffsetFromCenter_{ 0.0f, 0.0f };
    bool dotOffsetManual_ = false;

    // 自動配置時のYオフセット比率（セル高さに対する相対値）
    float dotYOffsetRatio_ = 0.0f; // 例: -0.03f で少し上寄せ

    // ドットの倍率（アセット 19x38 に乗算）
    float dotScale_ = 1.0f;
    bool dotScaleManual_ = false;
};