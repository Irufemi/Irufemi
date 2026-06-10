#pragma once

#include <cstdint>
#include "../../Core/Math/Vector2.h"

/**
 * @enum InputDeviceType
 * @brief 入力デバイスの種類
 */
enum class InputDeviceType {
    Keyboard,
    Mouse,
    GamePad
};

/**
 * @enum InputId
 * @brief 物理的な入力（キー、ボタン、軸）を一意に識別するためのID
 */
enum class InputId : uint16_t {
    Unknown = 0,

    // Keyboard (A-Z)
    Keyboard_A, Keyboard_B, Keyboard_C, Keyboard_D, Keyboard_E, Keyboard_F,
    Keyboard_G, Keyboard_H, Keyboard_I, Keyboard_J, Keyboard_K, Keyboard_L,
    Keyboard_M, Keyboard_N, Keyboard_O, Keyboard_P, Keyboard_Q, Keyboard_R,
    Keyboard_S, Keyboard_T, Keyboard_U, Keyboard_V, Keyboard_W, Keyboard_X,
    Keyboard_Y, Keyboard_Z,

    // Keyboard (Numbers)
    Keyboard_0, Keyboard_1, Keyboard_2, Keyboard_3, Keyboard_4,
    Keyboard_5, Keyboard_6, Keyboard_7, Keyboard_8, Keyboard_9,

    // Keyboard (Special)
    Keyboard_Space, Keyboard_Enter, Keyboard_Escape, Keyboard_Tab,
    Keyboard_Shift, Keyboard_Ctrl, Keyboard_Alt, Keyboard_Backspace,

    // Keyboard (Arrows)
    Keyboard_Up, Keyboard_Down, Keyboard_Left, Keyboard_Right,

    // Mouse Buttons
    Mouse_Left, Mouse_Right, Mouse_Middle,

    // Mouse Axes
    Mouse_X, Mouse_Y, Mouse_Wheel,

    // GamePad Buttons
    GamePad_A, GamePad_B, GamePad_X, GamePad_Y,
    GamePad_DPadUp, GamePad_DPadDown, GamePad_DPadLeft, GamePad_DPadRight,
    GamePad_Start, GamePad_Select,
    GamePad_LeftBumper, GamePad_RightBumper,
    GamePad_LeftThumbClick, GamePad_RightThumbClick,

    // GamePad Axes
    GamePad_LeftStickX, GamePad_LeftStickY,
    GamePad_RightStickX, GamePad_RightStickY,
    GamePad_LeftTrigger, GamePad_RightTrigger
};

/**
 * @struct InputActionValue
 * @brief アクションの現在値（1D/2Dアナログ、またはデジタル）を保持する構造体
 */
struct InputActionValue {
    float x = 0.0f;
    float y = 0.0f;

    /** @brief デジタル（ボタン）としての入力があるか */
    bool GetAsBool() const { return x != 0.0f || y != 0.0f; }
    
    /** @brief 1Dアナログ（トリガーや単一軸）としての値を取得 */
    float GetAsAxis1D() const { return x; }
    
    /** @brief 2Dアナログ（スティックやマウス移動）としての値を取得 */
    Vector2 GetAsAxis2D() const { return { x, y }; }
};

/**
 * @struct InputBinding
 * @brief どのアクションにどの物理入力を割り当てるかのバインディング情報
 */
struct InputBinding {
    InputId id = InputId::Unknown;
    /** 
     * @brief X軸に対するスケール値
     * @details 物理入力値を最終的なアクション値に変換する際の係数です。
     *          - 1.0f : そのまま（右や上などプラス方向）
     *          - -1.0f : 反転（左や下などマイナス方向）
     *          - 0.5f など : 感度を半分にする（マウスとパッドの感度合わせ等）
     */
    float scaleX = 1.0f;
    
    /** @brief Y軸に対するスケール値（用途は scaleX と同じ） */
    float scaleY = 1.0f;

    InputBinding() = default;
    InputBinding(InputId inputId, float sx = 1.0f, float sy = 1.0f)
        : id(inputId), scaleX(sx), scaleY(sy) {}
};
