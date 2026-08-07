#pragma once
#include "../Component.h"
#include <vector>

/**
 * @class CanvasComponent
 * @brief UI要素をグループ化し、一括で透明度を操作するキャンバスコンポーネント
 */
class CanvasComponent : public Component {
public:
    CanvasComponent() = default;
    ~CanvasComponent() override = default;

    /**
     * @brief Initialize を実行する。
     */
    void Initialize() override;
    /**
     * @brief Update を実行する。
     */
    void Update() override;
    
    /**
     * @brief ComponentName を取得する。
     * @return 取得された ComponentName
     */
    std::string GetComponentName() const override { return "CanvasComponent"; }
    /**
     * @brief OnRegisterProperties を実行する。
     */
    void OnRegisterProperties() override;

private:
    float groupAlpha_ = 1.0f; // グループ全体のアルファ値
};
