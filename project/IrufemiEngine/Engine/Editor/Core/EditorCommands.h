#pragma once

#ifdef EditorMode
#include "ICommand.h"
#include <functional>

/**
 * @class ChangeValueCommand
 * @brief 値の変更を記録し、Undo/Redo を行う汎用コマンド
 * @tparam T 変更する値の型
 */
template<typename T>
class ChangeValueCommand : public ICommand {
public:
    /**
     * @param oldValue 変更前の値
     * @param newValue 変更後の値
     * @param setter 値を適用するための関数
     */
    ChangeValueCommand(const T& oldValue, const T& newValue, std::function<void(const T&)> setter)
        : oldValue_(oldValue), newValue_(newValue), setter_(setter) {}

    void Do() override { setter_(newValue_); }
    void Undo() override { setter_(oldValue_); }

private:
    T oldValue_;
    T newValue_;
    std::function<void(const T&)> setter_;
};

#endif // EditorMode
