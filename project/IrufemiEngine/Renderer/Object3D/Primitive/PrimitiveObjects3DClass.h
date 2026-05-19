#include "../../Core/IRenderable.h"
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Engine/Core/Type/PrimitiveType.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Engine/Core/Math/Transform.h"

// 前方宣言
class Camera;
class TextureManager;
class DrawManager;
class DebugUI;
struct PrimitiveData;

/**
 * @class PrimitiveObjects3DClass
 * @brief 汎用的な3Dプリミティブ（立方体、球、平面など）を管理・描画するクラス
 * @details コンポーネント指向に基づき、メッシュ・マテリアル・トランスフォームの各機能を内部に持ちます。
 *          ImGuiエディタからのリアルタイムな形状変更やプロパティ編集に対応します。
 */
class PrimitiveObjects3DClass : public IRenderable {
public:
    /**
     * @struct TransformComponent
     * @brief 座標変換（位置・回転・スケール）を管理するコンポーネント
     */
    struct TransformComponent {
        Transform transform; //!< トランスフォーム情報
        bool isDirty = true; //!< 行列再計算が必要な場合のフラグ

        /**
         * @brief トランスフォーム情報をリソースへ反映し、行列を更新する
         * @param[out] resource 反映先のリソース
         * @param[in] camera 描画に使用するカメラ
         */
        void UpdateTransform(Object3DResource* resource, const Camera& camera);
    };

    /**
     * @struct MeshComponent
     * @brief メッシュ形状（頂点・インデックス情報）を管理するコンポーネント
     */
    struct MeshComponent {
        PrimitiveType type;                          //!< 現在のプリミティブ形状タイプ
        std::unique_ptr<Object3DResource> resource; //!< D3D12リソース

        /**
         * @brief 指定した形状タイプにメッシュを切り替える
         * @param[in] newType 新しい形状タイプ
         */
        void ChangeMesh(PrimitiveType newType);

        /**
         * @brief カスタムの PrimitiveData を用いて独自にメッシュリソースを再生成する
         * @param[in] data 再生成に使用する頂点・インデックスデータ
         */
        void ChangeMesh(const PrimitiveData& data);
    };

    /**
     * @struct MaterialComponent
     * @brief 色やテクスチャ、ライティング設定を管理するコンポーネント
     */
    struct MaterialComponent {
        std::string texturePath;      //!< テクスチャパス
        int selectedTextureIndex = 0; //!< ImGui選択用インデックス
        Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; //!< ベースカラー
        bool enableLighting = true;   //!< ライティングの有無
        int lightingMode = 3;         //!< ライティングモード (0:None, 1:Lambert, 2:Half-Lambert, 3:PBR)
        float metallic = 0.0f;        //!< 金属度
        float roughness = 0.5f;       //!< 粗さ
        
        Matrix4x4 uvTransform = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        }; //!< UV変換行列（スクロール・反転等用）
        float alphaReference = 0.0f;                       //!< ディスカード閾値
        int32_t useClampSampler = 0;                       //!< サンプラー切替 (0:WRAP, 1:CLAMP)

        /**
         * @brief マテリアル設定をリソースへ反映する
         * @param[out] resource 反映先のリソース
         * @param[in] textureManager テクスチャハンドル取得用
         */
        void UpdateMaterial(Object3DResource* resource, TextureManager* textureManager);
    };

public:
    PrimitiveObjects3DClass() = default;
    ~PrimitiveObjects3DClass() = default;

    /**
     * @brief 初期化処理
     * @param[in] camera 使用するカメラのポインタ
     * @param[in] type 初期形状タイプ
     * @param[in] texturePath 使用するテクスチャのパス
     */
    void Initialize(PrimitiveType type, const std::string& texturePath = "resources/uvChecker.png");

    /**
     * @brief 更新処理
     */
    void Update();

    /**
     * @brief 描画処理
     */
    void SyncBeforeDraw() override;
    void Draw() override;
    void DrawOutlineMask() override;

    /**
     * @brief 描画処理（カメラを外部から指定する場合）
     * @param[in] camera 描画に使用するカメラ
     */
    void Draw(const Camera& camera);

    /**
     * @brief ImGuiによるデバッグ・編集用UIを表示する
     * @param[in] label UIウィンドウおよび識別用のラベル
     */
    void Debug(const char* label = "Primitive Object");

    // --- 各コンポーネントへのアクセサ ---
    TransformComponent& GetTransform() { return transform_; }
    MeshComponent& GetMesh() { return mesh_; }
    MaterialComponent& GetMaterial() { return material_; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }

    // --- ヘルパーSetter ---
    void SetPosition(const Vector3& pos) { transform_.transform.translate = pos; transform_.isDirty = true; }
    void SetRotate(const Vector3& rot) { transform_.transform.rotate = rot; transform_.isDirty = true; }
    void SetScale(const Vector3& scale) { transform_.transform.scale = scale; transform_.isDirty = true; }
    void SetColor(const Vector4& color) { material_.color = color; }
    void SetTexture(const std::string& path) { material_.texturePath = path; }
    void SetShape(PrimitiveType type) { mesh_.ChangeMesh(type); transform_.isDirty = true; }
    void SetCustomPSO(ID3D12PipelineState* pso) { if (mesh_.resource) mesh_.resource->SetCustomPSO(pso); }
    void SetCustomCBVAddress(D3D12_GPU_VIRTUAL_ADDRESS addr) { if (mesh_.resource) mesh_.resource->SetCustomCBVAddress(addr); }

    /**
     * @brief カスタムの PrimitiveData を用いて現在のリソースを破棄し再初期化する
     * @param[in] data 再生成に使用する頂点・インデックスデータ
     */
    void ReinitializeMesh(const PrimitiveData& data);

    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    void SetCastShadows(bool cast) { castShadows_ = cast; }
    bool GetCastShadows() const { return castShadows_; }

    // --- 静的各種マネージャの設定 ---
    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetEngine(class IrufemiEngine* engine) { engine_ = engine; }

private:
    TransformComponent transform_; //!< トランスフォームコンポーネント
    MeshComponent mesh_;           //!< メッシュコンポーネント
    MaterialComponent material_;   //!< マテリアルコンポーネント
    bool isCullingEnabled_ = true; //!< 視錐台カリングの有効フラグ
    bool castShadows_ = true;      //!< 影を落とすフラグ

    // 静的ポインタ（既存の設計パターンを継承）
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    static class IrufemiEngine* engine_;
};



