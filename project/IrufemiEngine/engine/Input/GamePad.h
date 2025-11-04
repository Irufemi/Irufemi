#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <utility>
#pragma comment(lib, "Xinput.lib")

enum class Stick8 {
    Neutral, Up, UpRight, Right, DownRight, Down, DownLeft, Left, UpLeft
};

class GamePad {
public:
    GamePad() = default;
    ~GamePad() = default;

    void Initialize();
    void Update();

    // ボタン
    bool IsButtonDown(WORD button) const;
    bool IsButtonUp(WORD button) const;
    bool IsButtonPressed(WORD button) const;   // 立ち上がり
    bool IsButtonReleased(WORD button) const;  // 立ち下がり

    // アナログ
    float GetLeftStickX()  const;
    float GetLeftStickY()  const;
    float GetRightStickX() const;
    float GetRightStickY() const;
    float GetLeftTrigger() const;
    float GetRightTrigger() const;

    // デッドゾーン設定（必要なら外から変更可）
    void SetLeftDeadZone(float dz) { deadZoneLeft_ = dz; }
    void SetRightDeadZone(float dz) { deadZoneRight_ = dz; }

    // ★ 接続とインデックス
    bool IsConnected() const { return connected_; }
    void SetIndex(int idx) { index_ = idx; }

    // ★ 8方向（左スティック）
    Stick8 Left8(float threshold = 0.30f) const;
    bool Left8Is(Stick8 dir, float threshold = 0.30f) const;
    bool Left8Pressed(Stick8 dir, float threshold = 0.30f) const;
    bool Left8Released(Stick8 dir, float threshold = 0.30f) const;

    // 方向ショートカット
    bool LUp(float th = 0.30f) const;
    bool LDown(float th = 0.30f) const;
    bool LLeft(float th = 0.30f) const;
    bool LRight(float th = 0.30f) const;
    bool LUpRight(float th = 0.30f) const;
    bool LUpLeft(float th = 0.30f) const;
    bool LDownRight(float th = 0.30f) const;
    bool LDownLeft(float th = 0.30f) const;

    // ★ D-Pad 8方向
    Stick8 DPad8Now() const;
    bool DPad8Is(Stick8 dir) const;
    bool DPad8Pressed(Stick8 dir) const;
    bool DPad8Released(Stick8 dir) const;

    // D-Pad ショートカット
    bool DPadUp() const;    bool DPadUpPressed() const;    bool DPadUpReleased() const;
    bool DPadDown() const;  bool DPadDownPressed() const;  bool DPadDownReleased() const;
    bool DPadLeft() const;  bool DPadLeftPressed() const;  bool DPadLeftReleased() const;
    bool DPadRight() const; bool DPadRightPressed() const; bool DPadRightReleased() const;

    // 斜め
    bool DPadUpRightDown() const;   bool DPadUpRightPressed() const;   bool DPadUpRightReleased() const;
    bool DPadUpLeftDown() const;    bool DPadUpLeftPressed() const;    bool DPadUpLeftReleased() const;
    bool DPadDownRightDown() const; bool DPadDownRightPressed() const; bool DPadDownRightReleased() const;
    bool DPadDownLeftDown() const;  bool DPadDownLeftPressed() const;  bool DPadDownLeftReleased() const;

    // ★ トリガ（アナログ＆しきい値デジタル）
    float LeftTriggerAnalog(float deadzone = 30.0f)  const;
    float RightTriggerAnalog(float deadzone = 30.0f) const;
    bool LeftTriggerDown(uint8_t threshold = 30) const;
    bool LeftTriggerPressed(uint8_t threshold = 30) const;
    bool LeftTriggerReleased(uint8_t threshold = 30) const;
    bool RightTriggerDown(uint8_t threshold = 30) const;
    bool RightTriggerPressed(uint8_t threshold = 30) const;
    bool RightTriggerReleased(uint8_t threshold = 30) const;
    bool TriggerDown(bool right, uint8_t threshold = 30) const;
    bool TriggerPressed(bool right, uint8_t threshold = 30) const;
    bool TriggerReleased(bool right, uint8_t threshold = 30) const;

    // ★ RB/LB ショートカット
    bool LBDown() const;     bool LBPressed() const;     bool LBReleased() const;
    bool RBDown() const;     bool RBPressed() const;     bool RBReleased() const;

    // （任意）Y 反転
    void SetInvertY(bool inv) { invertY_ = inv; }

private:

    // ★ ラジアル正規化（円形DZ）: Controller と同等の内部ヘルパ
    static std::pair<float, float> RadialNormalize(short x, short y, int dz);

    // ★ 8方向化ヘルパ
    static Stick8 Stick8From(short x, short y, int dz, bool invertY, float threshold);

    XINPUT_STATE state_{};
    XINPUT_STATE prev_{};
    float deadZoneLeft_ = 0.2f;
    float deadZoneRight_ = 0.2f;
    bool connected_ = false;
    int  index_ = 0;
    bool invertY_ = false;
};
