#pragma once

class GameObject;

/**
 * @class Component
 * @brief すべてのコンポーネントの基底クラス
 * @details GameObjectにアタッチされ、初期化・更新・描画のライフサイクルを持ちます。
 */
class Component {
public:
    virtual ~Component() = default;

    /**
     * @brief コンポーネントの初期化
     */
    virtual void Initialize() {}

    /**
     * @brief 毎フレームの更新処理
     */
    virtual void Update() {}

    /**
     * @brief 描画処理（レンダラー系コンポーネントでオーバーライド）
     */
    virtual void Draw() {}

#ifdef EditorMode
    /**
     * @brief エディタ（インスペクター）用UIの描画処理
     * @details ImGuiを用いてパラメータの編集UIを構築します。
     */
    virtual void OnInspectorGUI() {}
#endif

    /**
     * @brief 所属するGameObjectのセット
     * @param gameObject 親となるGameObjectのポインタ
     */
    void SetGameObject(GameObject* gameObject) { gameObject_ = gameObject; }

    /**
     * @brief 所属するGameObjectの取得
     * @return GameObject* 親のポインタ
     */
    GameObject* GetGameObject() const { return gameObject_; }

protected:
    GameObject* gameObject_ = nullptr; ///< 親GameObjectへのポインタ
};
