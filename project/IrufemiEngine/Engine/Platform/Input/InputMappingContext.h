#pragma once

#include "InputAction.h"
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @class InputMappingContext
 * @brief アクションと物理入力（バインディング）の対応付けを管理するクラス
 */
class InputMappingContext {
public:
    InputMappingContext() = default;
    ~InputMappingContext() = default;

    /**
     * @brief アクションに対して新しいバインディングを追加する
     * @param[in] actionName アクション名（例: "Jump", "Move"）
     * @param[in] binding 割り当てる物理入力の設定
     */
    void AddBinding(const std::string& actionName, const InputBinding& binding);

    /**
     * @brief 指定したアクション名に紐づくすべてのバインディングを取得する
     * @param[in] actionName アクション名
     * @return バインディングのリスト（存在しない場合は空のリストを返す）
     */
    const std::vector<InputBinding>& GetBindings(const std::string& actionName) const;

    /**
     * @brief 登録されているすべてのアクション名を取得する
     * @return アクション名のリスト
     */
    std::vector<std::string> GetAllActionNames() const;

    /**
     * @brief すべてのバインディングをクリアする
     */
    void Clear();

private:
    std::unordered_map<std::string, std::vector<InputBinding>> mappings_{};
};
