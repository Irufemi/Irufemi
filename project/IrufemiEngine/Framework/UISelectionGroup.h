#pragma once
#include <vector>
#include "UIAnimator.h"
#include "Engine/Core/Math/Vector4.h"

class Sprite;
class InputManager;

/**
 * @class UISelectionGroup
 * @brief 縦並びのメニュー項目など、複数のSpriteから一つを選択するためのコントローラー
 * @details 上下キーでのインデックス切り替え、選択中の項目の明滅（色変更）、決定入力の管理を全自動で行います。
 */
class UISelectionGroup {
public:
    UISelectionGroup();
    ~UISelectionGroup() = default;

    /**
     * @brief 選択項目となるスプライトをリストの末尾に追加する
     * @param sprite 管理対象のスプライト
     */
    void AddItem(Sprite* sprite);

    /**
     * @brief 選択中項目の基本色を設定する
     * @param color 色（RGBA）
     */
    void SetActiveBaseColor(const Vector4& color) { activeBaseColor_ = color; }

    /**
     * @brief 非選択中項目の色を設定する
     * @param color 色（RGBA）
     */
    void SetInactiveColor(const Vector4& color) { inactiveColor_ = color; }

    /**
     * @brief 状態をリセットする（ポーズ再開時などに呼ぶ）
     */
    void Reset();

    /**
     * @brief 更新処理（キー入力によるカーソル移動、アニメーション更新）
     * @param input InputManagerへのポインタ
     */
    void Update(InputManager* input);

    /**
     * @brief 描画処理（選択状態に応じた色を自動適用して描画する）
     */
    void Draw();

    /**
     * @brief 現在選択されている項目のインデックスを取得
     */
    int GetSelectedIndex() const { return selectedIndex_; }

    /**
     * @brief 決定キー（Space または Enter）が押されたかを判定する
     */
    bool IsDecided() const { return isDecided_; }

private:
    std::vector<Sprite*> items_;
    int selectedIndex_ = 0;
    
    UIAnimator animator_;
    
    Vector4 activeBaseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 inactiveColor_ = {0.3f, 0.3f, 0.3f, 0.9f};
    
    bool isDecided_ = false;
};
