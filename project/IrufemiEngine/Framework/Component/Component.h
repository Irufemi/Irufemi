#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

class GameObject;
#include "Renderer/System/Core/IRenderable.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
namespace Irufemi { struct Ray; }

enum class ComponentPropertyType { Float, Float2, Float3, Float4, Int, Bool, String, Float3Array, Header, Separator, Enum, GameObjectRef };

struct ComponentProperty {
    std::string name;
    ComponentPropertyType type;
    void* data;
    float minVal = 0.0f;
    float maxVal = 0.0f;
    std::vector<std::string> enumNames;
    std::string tooltip = "";
    nlohmann::json defaultValue;

    ComponentProperty& SetTooltip(const std::string& text) {
        tooltip = text;
        return *this;
    }

    ComponentProperty& SetMinMax(float min, float max) {
        minVal = min;
        maxVal = max;
        return *this;
    }
};

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
     * @brief 開始処理 (最初のUpdateの直前に一度だけ呼ばれる)
     */
    virtual void Start() {}

    /**
     * @brief コンポーネントが有効化された時に呼ばれる
     */
    virtual void OnEnable() {}

    /**
     * @brief コンポーネントが無効化された時に呼ばれる
     */
    virtual void OnDisable() {}

    /**
     * @brief 毎フレームの更新処理
     */
    virtual void Update() {}

    /**
     * @brief Editモード時にUpdateを実行するかどうか
     * @details デフォルトはfalse（実行しない）。エディタでのプレビューが必要なコンポーネントはtrueを返す。
     */
    virtual bool CanUpdateInEditMode() const { return false; }

    /**
     * @brief ポーズ時（TimeScale == 0.0f）にUpdateを実行するかどうか
     * @details デフォルトはfalse（ポーズ中は実行しない）。UIやエフェクトなど、ポーズ中でもアニメーションさせたい場合はtrueを返す。
     */
    virtual bool CanUpdateWhenPaused() const { return false; }

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
     * @brief 衝突判定コールバック
     * @details 他のColliderと衝突した瞬間に呼ばれる
     */
    virtual void OnCollisionEnter(class GameObject* hitObject) {}

    /**
     * @brief 衝突判定コールバック
     * @details 他のColliderと衝突している間呼ばれ続ける
     */
    virtual void OnCollisionStay(class GameObject* hitObject) {}

    /**
     * @brief 衝突判定コールバック
     * @details 他のColliderとの衝突が終わった瞬間に呼ばれる
     */
    virtual void OnCollisionExit(class GameObject* hitObject) {}

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
    virtual bool Raycast(const Irufemi::Ray& ray, float& outDistance) const { return false; }

    /**
     * @brief 自身の持つ変数をリフレクションシステムに登録する
     */
    virtual void OnRegisterProperties() {}

    /**
     * @brief コンポーネントの種類を表す文字列を返す
     */
    virtual std::string GetComponentName() const { return "Component"; }

    /**
     * @brief IDが再生成された際（Clone等）に、内部で保持しているID参照を新しいIDに読み替えるためのコールバック
     * @param idMap 古いID(Key) と 新しいID(Value) の対応表
     */
    virtual void OnIDRemapped(const std::unordered_map<uint64_t, uint64_t>& idMap) {}

    /**
     * @brief 登録されたプロパティリストを取得する
     */
    const std::vector<ComponentProperty>& GetProperties() const { return properties_; }

    /**
     * @brief プロパティの登録ヘルパー
     */
    ComponentProperty& RegisterProperty(const std::string& name, float* ptr) { properties_.push_back({name, ComponentPropertyType::Float, ptr, 0.0f, 0.0f, {}, "", *ptr}); return properties_.back(); }
    ComponentProperty& RegisterPropertyRange(const std::string& name, float* ptr, float min, float max) { properties_.push_back({name, ComponentPropertyType::Float, ptr, min, max, {}, "", *ptr}); return properties_.back(); }
    ComponentProperty& RegisterProperty(const std::string& name, int* ptr) { properties_.push_back({name, ComponentPropertyType::Int, ptr, 0.0f, 0.0f, {}, "", *ptr}); return properties_.back(); }
    ComponentProperty& RegisterPropertyRange(const std::string& name, int* ptr, int min, int max) { properties_.push_back({name, ComponentPropertyType::Int, ptr, static_cast<float>(min), static_cast<float>(max), {}, "", *ptr}); return properties_.back(); }
    ComponentProperty& RegisterEnum(const std::string& name, int* ptr, const std::vector<std::string>& enumNames) { properties_.push_back({name, ComponentPropertyType::Enum, ptr, 0.0f, 0.0f, enumNames, "", *ptr}); return properties_.back(); }
    ComponentProperty& RegisterProperty(const std::string& name, bool* ptr) { properties_.push_back({name, ComponentPropertyType::Bool, ptr, 0.0f, 0.0f, {}, "", *ptr}); return properties_.back(); }
    ComponentProperty& RegisterProperty(const std::string& name, std::string* ptr) { properties_.push_back({name, ComponentPropertyType::String, ptr, 0.0f, 0.0f, {}, "", *ptr}); return properties_.back(); }
    ComponentProperty& RegisterGameObjectRef(const std::string& name, uint64_t* ptr) { properties_.push_back({name, ComponentPropertyType::GameObjectRef, ptr, 0.0f, 0.0f, {}, "", *ptr}); return properties_.back(); }
    ComponentProperty& RegisterProperty(const std::string& name, Irufemi::Vector2* ptr) { properties_.push_back({name, ComponentPropertyType::Float2, ptr, 0.0f, 0.0f, {}, "", nlohmann::json{ptr->x, ptr->y}}); return properties_.back(); }
    ComponentProperty& RegisterProperty(const std::string& name, Irufemi::Vector3* ptr) { properties_.push_back({name, ComponentPropertyType::Float3, ptr, 0.0f, 0.0f, {}, "", nlohmann::json{ptr->x, ptr->y, ptr->z}}); return properties_.back(); }
    ComponentProperty& RegisterProperty(const std::string& name, Irufemi::Vector4* ptr) { properties_.push_back({name, ComponentPropertyType::Float4, ptr, 0.0f, 0.0f, {}, "", nlohmann::json{ptr->x, ptr->y, ptr->z, ptr->w}}); return properties_.back(); }
    ComponentProperty& RegisterProperty(const std::string& name, std::vector<Irufemi::Vector3>* ptr) { 
        nlohmann::json jArray = nlohmann::json::array();
        for (const auto& v : *ptr) jArray.push_back({ v.x, v.y, v.z });
        properties_.push_back({name, ComponentPropertyType::Float3Array, ptr, 0.0f, 0.0f, {}, "", jArray}); 
        return properties_.back(); 
    }
    ComponentProperty& RegisterHeader(const std::string& name) { properties_.push_back({name, ComponentPropertyType::Header, nullptr, 0.0f, 0.0f, {}, "", nullptr}); return properties_.back(); }
    ComponentProperty& RegisterSeparator() { properties_.push_back({"", ComponentPropertyType::Separator, nullptr, 0.0f, 0.0f, {}, "", nullptr}); return properties_.back(); }

    /**
     * @brief コンポーネントの状態をJSONにシリアライズする
     */
    virtual nlohmann::json Serialize() { 
        nlohmann::json j = nlohmann::json::object(); 
        for (const auto& prop : properties_) {
            switch (prop.type) {
                case ComponentPropertyType::Float: j[prop.name] = *static_cast<float*>(prop.data); break;
                case ComponentPropertyType::Enum:
                case ComponentPropertyType::Int: j[prop.name] = *static_cast<int*>(prop.data); break;
                case ComponentPropertyType::Bool: j[prop.name] = *static_cast<bool*>(prop.data); break;
                case ComponentPropertyType::String: j[prop.name] = *static_cast<std::string*>(prop.data); break;
                case ComponentPropertyType::GameObjectRef: j[prop.name] = *static_cast<uint64_t*>(prop.data); break;
                case ComponentPropertyType::Float2: {
                    auto* v = static_cast<Irufemi::Vector2*>(prop.data);
                    j[prop.name] = { v->x, v->y };
                    break;
                }
                case ComponentPropertyType::Float3: {
                    auto* v = static_cast<Irufemi::Vector3*>(prop.data);
                    j[prop.name] = { v->x, v->y, v->z };
                    break;
                }
                case ComponentPropertyType::Float4: {
                    auto* v = static_cast<Irufemi::Vector4*>(prop.data);
                    j[prop.name] = { v->x, v->y, v->z, v->w };
                    break;
                }
                case ComponentPropertyType::Float3Array: {
                    auto* arr = static_cast<std::vector<Irufemi::Vector3>*>(prop.data);
                    nlohmann::json jArray = nlohmann::json::array();
                    for (const auto& v : *arr) {
                        jArray.push_back({ v.x, v.y, v.z });
                    }
                    j[prop.name] = jArray;
                    break;
                }
                case ComponentPropertyType::Header:
                case ComponentPropertyType::Separator:
                    break;
            }
        }
        return j; 
    }

    /**
     * @brief JSONからコンポーネントの状態を復元する
     */
    virtual void Deserialize(const nlohmann::json& j) { 
        for (const auto& prop : properties_) {
            if (!j.contains(prop.name)) continue;
            switch (prop.type) {
                case ComponentPropertyType::Float: *static_cast<float*>(prop.data) = j[prop.name].get<float>(); break;
                case ComponentPropertyType::Enum:
                case ComponentPropertyType::Int: *static_cast<int*>(prop.data) = j[prop.name].get<int>(); break;
                case ComponentPropertyType::Bool: *static_cast<bool*>(prop.data) = j[prop.name].get<bool>(); break;
                case ComponentPropertyType::String: *static_cast<std::string*>(prop.data) = j[prop.name].get<std::string>(); break;
                case ComponentPropertyType::GameObjectRef: *static_cast<uint64_t*>(prop.data) = j[prop.name].get<uint64_t>(); break;
                case ComponentPropertyType::Float2: {
                    auto* v = static_cast<Irufemi::Vector2*>(prop.data);
                    auto arr = j[prop.name];
                    if (arr.is_array() && arr.size() >= 2) {
                        v->x = arr[0].get<float>(); v->y = arr[1].get<float>();
                    }
                    break;
                }
                case ComponentPropertyType::Float3: {
                    auto* v = static_cast<Irufemi::Vector3*>(prop.data);
                    auto arr = j[prop.name];
                    if (arr.is_array() && arr.size() >= 3) {
                        v->x = arr[0].get<float>(); v->y = arr[1].get<float>(); v->z = arr[2].get<float>();
                    }
                    break;
                }
                case ComponentPropertyType::Float4: {
                    auto* v = static_cast<Irufemi::Vector4*>(prop.data);
                    auto arr = j[prop.name];
                    if (arr.is_array() && arr.size() >= 4) {
                        v->x = arr[0].get<float>(); v->y = arr[1].get<float>(); v->z = arr[2].get<float>(); v->w = arr[3].get<float>();
                    }
                    break;
                }
                case ComponentPropertyType::Float3Array: {
                    auto* vecArr = static_cast<std::vector<Irufemi::Vector3>*>(prop.data);
                    auto arr = j[prop.name];
                    if (arr.is_array()) {
                        vecArr->clear();
                        for (const auto& item : arr) {
                            if (item.is_array() && item.size() >= 3) {
                                vecArr->push_back({item[0].get<float>(), item[1].get<float>(), item[2].get<float>()});
                            }
                        }
                    }
                    break;
                }
                case ComponentPropertyType::Header:
                case ComponentPropertyType::Separator:
                    break;
            }
        }
    }

    /**
     * @brief エディタ（インスペクター）用UIの描画処理について
     * @details エディタ側のカスタムUIを実装したい場合は、OnRegisterProperties() をオーバーライドしてプロパティを登録するか、
     * Engine/Editor/Core/ComponentEditorRegistry にカスタムエディタ(IComponentEditor)を登録してください。
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
    std::vector<ComponentProperty> properties_; ///< 自動シリアライズ・UI化用のプロパティリスト
};
