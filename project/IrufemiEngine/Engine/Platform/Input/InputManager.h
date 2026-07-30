#pragma once
#include <memory>
#include "Keyboard.h"
#include "GamePad.h"
#include "Mouse.h"
#include "../../Core/Math/Vector2.h"
#include "InputMappingContext.h"
#include <string>
#include <unordered_map>

// 役割：具体実装(Keyboard/GamePad/Mouse)を保持し、旧APIをフォワードして互換を維持するファサード
/**
 * @class InputManager
 * @brief キーボード、マウス、ゲームパッドの入力を一括管理するクラス
 * @details 各入力デバイスの具体的なインスタンスを保持し、統一したインターフェースを提供します。
 *          既存コードとの互換性を維持するためのフォワードメソッドも備えています。
 */
class InputManager {
public:
    InputManager() = default;
    ~InputManager() = default;

    /** @name 初期化・更新 */
    ///@{
    /**
     * @brief 初期化処理
     * @param[in] hwnd ウィンドウハンドル
     */
    void Initialize(HWND hwnd);

    /**
     * @brief 毎フレームの更新処理
     */
    void Update();
    ///@}

    /** @name アクションベース入力（推奨API） */
    ///@{
    /**
     * @brief アクションに物理入力をバインドする
     * @param[in] actionName アクション名（例: "Jump", "MoveX"）
     * @param[in] inputId 割り当てる物理入力（InputId::Keyboard_Space など）
     * @param[in] scale 物理入力値を最終値に変換する際の係数（1.0f=そのまま, -1.0f=反転, 0.5f=感度半減 など）
     */
    void BindAction(const std::string& actionName, InputId inputId, float scale = 1.0f);

    /** @brief 指定アクションのアナログ値（1D/2D）を取得する */
    InputActionValue GetActionValue(const std::string& actionName) const;

    /** @brief 指定アクションが押されているか（Down） */
    bool IsActionDown(const std::string& actionName) const;
    /** @brief 指定アクションが押された瞬間か（Triggered/Pressed） */
    bool IsActionTriggered(const std::string& actionName) const;
    /** @brief 指定アクションが離された瞬間か（Released） */
    bool IsActionReleased(const std::string& actionName) const;

    /** @brief 全てのアクションバインディングを解除する */
    void ClearActionBindings();
    ///@}

    /** @name デバイス取得（推奨API） */
    ///@{
    Keyboard* GetKeyboard() { return keyboard_.get(); }
    GamePad*  GetGamePad()  { return gamepad_.get(); }
    Mouse*    GetMouse()    { return mouse_.get(); }
    ///@}

    /** @name キーボード入力（互換用API） */
    /** @name キー状態の取得 */
    ///@{
    bool IsKeyDown(uint8_t key) const;
    bool IsKeyUp(uint8_t key) const;
    /** @brief キーが押された瞬間か判定（立ち上がり） */
    bool IsKeyPressed(uint8_t key) const;
    /** @brief キーが離された瞬間か判定（立ち下がり） */
    bool IsKeyReleased(uint8_t key) const;
    ///@}

    /** @name DIK互換API */
    ///@{
    bool IsKeyDownDIK(uint8_t dik) const;
    bool IsKeyUpDIK(uint8_t dik) const;
    bool IsKeyPressedDIK(uint8_t dik) const;
    bool IsKeyReleasedDIK(uint8_t dik) const;
    ///@}
    ///@}

    /** @name ゲームパッド入力（互換用API） */
    ///@{
    /** @name ボタン入力状態 */
    ///@{
    bool IsButtonDown(WORD button) const;
    bool IsButtonUp(WORD button) const;
    /** @brief ボタンが押された瞬間か判定 */
    bool IsButtonPressed(WORD button) const;
    /** @brief ボタンが離された瞬間か判定 */
    bool IsButtonReleased(WORD button) const;
    ///@}

    float GetLeftStickX()  const;
    float GetLeftStickY()  const;
    float GetRightStickX() const;
    float GetRightStickY() const;

    float GetLeftTrigger()  const;
    float GetRightTrigger() const;

    bool StartDown()     const;
    bool StartPressed()  const;
    bool StartReleased() const;

    bool DPadUp()    const;
    bool DPadDown()  const;
    bool DPadLeft()  const;
    bool DPadRight() const;
    bool DPadUpPressed()    const;
    bool DPadDownPressed()  const;
    bool DPadLeftPressed()  const;
    bool DPadRightPressed() const;
    ///@}

    /** @name マウス入力（互換用API） */
    ///@{
    bool IsMouseButtonDown(Mouse::Button button)     const;
    bool IsMouseButtonPressed(Mouse::Button button)  const;
    bool IsMouseButtonReleased(Mouse::Button button) const;
    const Irufemi::Vector2& GetMousePosition()   const;
    const Irufemi::Vector2& GetMouseDelta()      const;
    float GetMouseWheelDelta()          const;
    
    /** @brief エディタ用：仮想的なマウスローカル座標を上書き設定する */
    void SetVirtualMousePosition(const Irufemi::Vector2& pos, bool enable) {
        if (mouse_) mouse_->SetVirtualPosition(pos, enable);
    }
    ///@}

private:
    /** @brief 物理入力デバイスから現在の状態（アナログ値または0/1）を取得する内部関数 */
    float GetPhysicalInputValue(InputId id) const;

    std::unique_ptr<Keyboard> keyboard_{};
    std::unique_ptr<GamePad>  gamepad_{};
    std::unique_ptr<Mouse>    mouse_{};
    HWND hwnd_ = nullptr;

    InputMappingContext mappingContext_{};
    
    // 前フレームと現在のフレームのアクション値を保持（Triggered等の判定用）
    std::unordered_map<std::string, InputActionValue> currentActionValues_{};
    std::unordered_map<std::string, InputActionValue> previousActionValues_{};
};
