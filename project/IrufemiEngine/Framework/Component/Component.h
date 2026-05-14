#pragma once
#include <nlohmann/json.hpp>
#include <string>

class GameObject;
#include "Renderer/Core/IRenderable.h"
struct Ray;

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

    /**
     * @brief 紐づく Renderable オブジェクトを取得する
     * @details レンダラー系コンポーネントがこれをオーバーライドすることで、アウトライン描画などを共通化します。
     */
    virtual IRenderable* GetRenderable() { return nullptr; }

    /**
     * @brief 選択中の輪郭マスク描画処理
     */
    virtual void DrawOutlineMask() {
        if (auto renderable = GetRenderable()) {
            renderable->DrawOutlineMask();
        }
    }

    /**
     * @brief レイキャスト判定（エディタでのオブジェクト選択時などに呼ばれる）
     * @param[in] ray 判定するレイ
     * @param[out] outDistance レイの始点から衝突点までの距離
     * @return 衝突した場合は true
     */
    virtual bool Raycast(const Ray& ray, float& outDistance) const { return false; }

    /**
     * @brief コンポーネントの種類を表す文字列を返す
     */
    virtual std::string GetComponentName() const { return "Component"; }

    /**
     * @brief コンポーネントの状態をJSONにシリアライズする
     */
    virtual nlohmann::json Serialize() { return nlohmann::json::object(); }

    /**
     * @brief JSONからコンポーネントの状態を復元する
     */
    virtual void Deserialize(const nlohmann::json& j) { (void)j; }

    /**
     * @brief エディタ（インスペクター）用UIの描画処理
     * @details ImGuiを用いてパラメータの編集UIを構築します。
     */


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
