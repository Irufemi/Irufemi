#pragma once

#ifdef EditorMode
class Component;

/**
 * @class IComponentEditor
 * @brief 特定のコンポーネントのInspectorUIを描画するクラスの基底インターフェース
 */
class IComponentEditor {
public:
    virtual ~IComponentEditor() = default;

    /**
     * @brief 渡されたコンポーネントのプロパティをImGuiで描画する
     * @param[in] component 描画対象の基底コンポーネントポインタ
     */
    virtual void Draw(Component* component) = 0;
};
#endif // EditorMode
