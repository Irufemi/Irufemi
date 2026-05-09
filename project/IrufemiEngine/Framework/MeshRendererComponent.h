#pragma once
#include "Component.h"
#include <memory>
#include <string>

// 前方宣言
class ObjClass;
class TransformComponent;

class MeshRendererComponent : public Component {
public:
    MeshRendererComponent();
    ~MeshRendererComponent() override;

    // 初期化時にモデルファイル名を指定
    void LoadModel(const std::string& filename);

    void Initialize() override;
    void Update() override;
    void Draw() override;

#ifdef EditorMode
    void OnInspectorGUI() override;
#endif

private:
    std::unique_ptr<ObjClass> obj_;                 ///< 実際の描画を担う既存クラス
    TransformComponent* transform_ = nullptr;       ///< 親のTransform情報（キャッシュ）
    std::string modelName_ = "plane.obj";           ///< 読み込むモデル名
};
