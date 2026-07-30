#pragma once
#include "IScene.h"
#include <memory>
#include <vector>
#include <cstdint>
#include <string>
#include <mutex>
#include <unordered_map>
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Graphics/Camera/OrbitCameraController.h"

// 前方宣言
class IrufemiEngine;
class CameraManager;
class DebugCamera;
struct DirectionalLight;
struct PointLight;
struct SpotLight;
struct AreaLight;
class GameObject;
class Camera;

/**
 * @class BaseScene
 * @brief すべてのゲームシーンの基底クラス。必須オブジェクト（カメラ、ライト等）を統合管理する。
 */
class BaseScene : public IScene {
public:
    BaseScene();
    virtual ~BaseScene();

    // --- 基本サイクル関数 ---

    /**
     * @brief シーンの初期化。継承先で必ず基底クラスの Initialize を呼んでください。
     */
    virtual void Initialize(IrufemiEngine* engine) override;
    
    /**
     * @brief 毎フレームの更新。継承先はこれを呼び出すことで、カメラ等の共通更新が行われます。
     */
    virtual void Update() override;
    
    /**
     * @brief 毎フレームの描画処理。継承先から呼び出すと GameObject の Draw が自動実行されます。
     */
    virtual void Draw() override;

    /**
     * @brief シーンに GameObject を追加する
     */
    void AddGameObject(std::shared_ptr<GameObject> obj);

    /**
     * @brief シーンの指定した位置に GameObject を挿入する (Undo用)
     */
    void InsertGameObject(std::shared_ptr<GameObject> obj, size_t index);

    /**
     * @brief シーンから GameObject を削除する
     */
    void RemoveGameObject(std::shared_ptr<GameObject> obj);

    /**
     * @brief シーン内のすべての GameObject をクリアする（Playモード終了時などの復元用）
     */
    void ClearGameObjects();

    /**
     * @brief プレハブ（JSON）からオブジェクトを動的に生成し、シーンに追加する
     * @param prefabPath プレハブのファイルパス
     * @param position 初期座標
     * @return 生成された GameObject のポインタ
     */
    std::shared_ptr<GameObject> InstantiatePrefab(const std::string& prefabPath, const Irufemi::Vector3& position = {0,0,0});

    /**
     * @brief シーンが保持する GameObject のリストを取得する
     */
    const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const override { return gameObjects_; }

    /**
     * @brief オブジェクトの現在のインデックスを取得する (Undo用)
     */
    size_t GetGameObjectIndex(std::shared_ptr<GameObject> obj) const;

    /**
     * @brief 生ポインタから該当する shared_ptr の GameObject を探して返す
     */
    std::shared_ptr<GameObject> FindGameObject(GameObject* ptr) {
        for (auto& obj : gameObjects_) {
            if (obj.get() == ptr) return obj;
        }
        return nullptr;
    }

    /**
     * @brief オブジェクトの名前から該当する shared_ptr の GameObject を探して返す（O(1)検索・最初に見つかったものを返す）
     */
    std::shared_ptr<GameObject> FindGameObject(const std::string& name);

    /**
     * @brief オブジェクトのインスタンスIDから該当する shared_ptr の GameObject を探して返す（O(1)検索）
     */
    std::shared_ptr<GameObject> FindGameObjectByID(uint64_t id);

    /**
     * @brief 指定した名前を持つすべての GameObject を返す
     */
    std::vector<std::shared_ptr<GameObject>> FindGameObjects(const std::string& name);



    /**
     * @brief 指定したタグを持つ全てのGameObjectを取得する
     */
    std::vector<std::shared_ptr<GameObject>> FindGameObjectsWithTag(const std::string& tag);

    /**
     * @brief シーン内で一意となるGameObject名を生成する (エディタ・複製用)
     */
    std::string GetUniqueObjectName(const std::string& baseName);

    /**
     * @brief GameObjectの名前が変更された際にインデックスを更新するためのコールバック
     */
    void OnGameObjectNameChanged(const std::shared_ptr<GameObject>& obj, const std::string& oldName, const std::string& newName);

    // --- ライフサイクル関数 ---
    
    /**
     * @brief シーンの終了処理。リソースの明示的な解放などを行います。
     */
    virtual void Finalize() override {}

    /**
     * @brief シーンが最前面でアクティブになった時に呼ばれます。
     */
    virtual void OnEnter() override {}

    /**
     * @brief シーンが破棄される直前、または完全に非アクティブになる時に呼ばれます。
     */
    virtual void OnExit() override {}

    /**
     * @brief 上に別のシーンがPushされ、このシーンがバックグラウンドに回った時に呼ばれます。
     */
    virtual void OnSuspend() override {}

    /**
     * @brief 上のシーンがPopされ、このシーンが再び最前面に復帰した時に呼ばれます。
     */
    virtual void OnResume() override {}

    /**
     * @brief IrufemiEngine のインスタンスを取得する
     */
    IrufemiEngine* GetEngine() const { return engine_; }

    // --- デバッグ機能 ---
    
    /**
     * @brief 共通のデバッグタブ描画。
     */
    virtual void DrawDebugTab() override;

    // --- シリアライズ機能 ---
    virtual nlohmann::json Serialize() const override;
    virtual void Deserialize(const nlohmann::json& j) override;

protected:
    IrufemiEngine* engine_ = nullptr;

    // --- オブジェクト管理 ---
    std::recursive_mutex sceneMutex_;
    std::vector<std::shared_ptr<GameObject>> gameObjects_;
    std::vector<std::shared_ptr<GameObject>> pendingAdds_;
    std::vector<std::shared_ptr<GameObject>> pendingRemoves_;
    
    // 高速検索(O(1))用インデックス
    std::unordered_map<std::string, std::vector<std::weak_ptr<GameObject>>> nameIndex_;
    std::unordered_map<uint64_t, std::weak_ptr<GameObject>> idIndex_;

    // デバッグ用カメラフラグ
    bool isDebugCameraMode_ = false;
    std::string previousActiveCameraName_ = "Main";

    // デバッグ用の独立したカメラインスタンスとそのコントローラー
    std::shared_ptr<Camera> debugCamera_;
    std::unique_ptr<OrbitCameraController> debugCameraController_;

    // --- ライティング ---
    std::unique_ptr<DirectionalLight> directionalLight_;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;

    // --- フレームデータの自動送信 ---
    void SubmitFrameData();

    // ── 入力ヘルパ ──
    // InputManager をラップした安全なヘルパー
    bool DownVK(uint8_t vk) const;
    bool PressedVK(uint8_t vk) const;
    bool ReleasedVK(uint8_t vk) const;

    bool DownDIK(uint8_t dik) const;
    bool PressedDIK(uint8_t dik) const;
    bool ReleasedDIK(uint8_t dik) const;

    // 互換性のため（既存の IsKeyPressed 等も呼び出しやすくする）
    bool IsKeyDown(uint8_t vk) const { return DownVK(vk); }
    bool IsKeyPressed(uint8_t vk) const { return PressedVK(vk); }
    bool IsButtonDown(unsigned short button) const;
    bool IsButtonPressed(unsigned short button) const;
};
