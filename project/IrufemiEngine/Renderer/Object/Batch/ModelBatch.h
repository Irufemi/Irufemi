#pragma once

#include "Renderer/System/Core/BaseBatch.h"
#include "Resource/Model/Data/ObjModel.h"

struct ManagedModel;
struct GpuMesh;
class ModelManager;

/**
 * @class ModelBatch
 * @brief 外部モデルデータ（.obj, .gltf）を描画するための領域クラス
 */
class ModelBatch : public BaseBatch {
public:
    ModelBatch() = default;
    ~ModelBatch() override = default;

    /**
     * @brief ModelManager を設定する。
     * @param[in] mm 設定する ModelManager の値
     */
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; }

    /**
     * @brief Initialize を実行する。
     */
    void Initialize(const std::string& objFilename);
    
    /**
     * @brief Draw を実行する。
     */
    void Draw() override;
    /**
     * @brief Draw を実行する。
     */
    void Draw(bool isUI);

    /**
     * @brief GpuMesh を取得する。
     * @return 取得された GpuMesh
     */
    const GpuMesh* GetGpuMesh() const; // 共有メッシュ取得

protected:
    /**
     * @brief BoundingSphereRadius を取得する。
     * @return 取得された BoundingSphereRadius
     */
    float GetBoundingSphereRadius() const override;

private:
    /**
     * @brief InitializeResources を実行する。
     */
    void InitializeResources();
    /**
     * @brief CreateMaterialResources を実行する。
     */
    void CreateMaterialResources(const ObjMesh& mesh);
    /**
     * @brief EnsureSharedTexture を実行する。
     */
    void EnsureSharedTexture(const ObjMesh& mesh);

private:
    static ModelManager* modelManager_;

    // 共有モデルデータ(CPU/GPU)
    ResourceHandle modelHandle_{};
    bool isResourcesInitialized_ = false;
};
