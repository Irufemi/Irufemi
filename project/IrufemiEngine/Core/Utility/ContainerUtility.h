#pragma once
#include <vector>
#include <algorithm>
#include <utility>

namespace Irufemi::Container {

/**
 * @brief 順序非依存の高速要素削除（Swap & Pop 手法）
 * @details 指定した値を配列から O(1) で削除します。順序は保持されません。
 *          逆順ループ走査中の末尾要素であれば探索なしで即時 pop します。
 * @tparam T 要素型
 * @tparam U 比較対象の型
 * @param vec 対象の std::vector
 * @param value 削除したい値
 * @return 削除された場合は true、見つからなかった場合は false
 */
template <typename T, typename U>
bool EraseSwap(std::vector<T>& vec, const U& value) {
    if (vec.empty()) {
        return false;
    }
    // 逆順ループ走査中の末尾要素なら探索不要で即時 pop (完全な O(1))
    if (vec.back() == value) {
        vec.pop_back();
        return true;
    }
    // 末尾以外の場合は末尾要素で上書きして pop_back (シフトなしの O(1))
    auto it = std::find(vec.begin(), vec.end(), value);
    if (it != vec.end()) {
        *it = std::move(vec.back());
        vec.pop_back();
        return true;
    }
    return false;
}

/**
 * @brief イテレータ指定での高速要素削除（Swap & Pop 手法）
 * @details 指定したイテレータ位置を末尾要素で上書きして pop_back します。順序は保持されません。
 * @tparam T 要素型
 * @param vec 対象の std::vector
 * @param it 削除したいイテレータ
 */
template <typename T>
void EraseSwap(std::vector<T>& vec, typename std::vector<T>::iterator it) {
    if (it == vec.end()) {
        return;
    }
    *it = std::move(vec.back());
    vec.pop_back();
}

/**
 * @brief 条件一致による順序非依存の高速要素削除（Swap & Pop 手法）
 * @details 述語条件に一致する最初の要素を O(1) で削除します。順序は保持されません。
 * @tparam T 要素型
 * @tparam Predicate 条件述語の型
 * @param vec 対象の std::vector
 * @param pred 判定関数またはラムダ式
 * @return 削除された場合は true、見つからなかった場合は false
 */
template <typename T, typename Predicate>
bool EraseSwapIf(std::vector<T>& vec, Predicate pred) {
    if (vec.empty()) {
        return false;
    }
    if (pred(vec.back())) {
        vec.pop_back();
        return true;
    }
    auto it = std::find_if(vec.begin(), vec.end(), pred);
    if (it != vec.end()) {
        *it = std::move(vec.back());
        vec.pop_back();
        return true;
    }
    return false;
}

} // namespace Irufemi::Container
