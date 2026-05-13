#pragma once

#ifdef EditorMode
#include <memory>
#include <string>

class GameObject;
class EditorManager;

/**
 * @class EditorActionManager
 * @brief エディタ経由でのGameObject生成・削除等のアクションを統括するマネージャ
 */
class EditorActionManager {
public:
    explicit EditorActionManager(EditorManager* editor);

    /**
     * @brief アセット（モデルや画像）のパスから適切なGameObjectを生成してシーンに追加する
     * @param[in] assetPath ドロップされたアセットの相対パス
     */
    void CreateObjectFromAsset(const std::string& assetPath);

    /**
     * @brief 空のGameObject、または指定したプリミティブを生成する
     * @param[in] typeName 生成する種類 ("Empty", "Cube", "Sprite" 等)
     */
    void CreatePrimitiveObject(const std::string& typeName);

    /**
     * @brief 指定したGameObjectを複製する
     * @param[in] target 複製元のGameObject
     */
    void DuplicateObject(std::shared_ptr<GameObject> target);

    /**
     * @brief 指定したGameObjectをシーンから削除し、選択を解除する
     * @param[in] target 削除対象のGameObject
     */
    void DeleteObject(std::shared_ptr<GameObject> target);

private:
    EditorManager* editorManager_ = nullptr;
};

#endif // EditorMode
