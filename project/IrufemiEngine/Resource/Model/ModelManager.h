#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <sstream>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>
#include "Resource/Model/Data/ObjModel.h"
#include "Resource/Model/Data/ModelData.h"
#include "Resource/Model/Data/MaterialData.h"
#include "Resource/Model/Data/VoxelizedModel.h"
#include "Engine/Core/Math/Geometry/Math.h"

// 前方宣言
struct aiNode;
namespace Assimp { class Importer; }
struct aiScene;
struct aiMesh;
struct aiMaterial;
struct Node;
class DirectXCommon;
class TextureManager;

/**
 * @struct GpuMesh
 * @brief GPU上に転送されたメッシュリソースを保持する構造体
 */
struct GpuMesh {
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView{};
    UINT vertexCount = 0;
    UINT indexCount = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE vertexSrvHandle{};
};

/**
 * @struct GpuMaterial
 * @brief GPU上で使用されるマテリアルリソースを保持する構造体
 */
struct GpuMaterial {
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle{};
};

/**
 * @struct ManagedModel
 * @brief CPU/GPU両方のデータを統合して管理する単位
 */
struct ManagedModel {
    std::shared_ptr<ObjModel> cpuModel;
    std::vector<std::shared_ptr<GpuMesh>> gpuMeshes;
    std::vector<std::shared_ptr<GpuMaterial>> gpuMaterials;
};

/**
 * @class ModelManager
 * @brief モデルリソース（OBJ, GLTF等）のロード、管理、キャッシュを行うマネージャクラス
 * 
 * 設計思想:
 * - 読み込み済みのモデルをファイルパス（またはファイル名）でキャッシュし、同一リソースの重複ロードを防ぎます。
 * - std::weak_ptr を用いたキャッシュ管理により、不要になったリソースの自動的な解放を支援します。
 * - Assimpライブラリを使用して多様な3Dモデルフォーマットに対応します。
 * 
 * 使い方:
 * 1. Initialize() でエンジン共通コンポーネントを登録します。
 * 2. SetRootDirectory() でリソースのベースパスを設定します（デフォルトは "resources/model"）。
 * 3. GetModel("filename.obj") でモデルを取得します。初回呼び出し時にロードが行われます。
 */
class ModelManager {
public:
    ModelManager() = default;
    ~ModelManager() = default;

    /**
     * @brief マネージャの初期化
     * @param dxCommon DirectXコンポーネントへのポインタ
     * @param textureManager テクスチャマネージャへのポインタ（マテリアルのテクスチャロードに使用）
     */
    void Initialize(DirectXCommon* dxCommon, TextureManager* textureManager);

    /**
     * @brief モデル検索のルートディレクトリを設定
     * @param root ルートディレクトリのパス
     */
    void SetRootDirectory(std::string root);

    /**
     * @brief モデルを取得する。キャッシュにあればそれを返し、なければロードする。
     * @param filename ファイル名（または相対パス）。拡張子を含む。
     * @return 取得した ManagedModel への共有ポインタ。失敗時は nullptr。
     */
    std::shared_ptr<ManagedModel> GetModel(const std::string& filename);

    /**
     * @brief 指定したフォルダ以下のモデルをすべて先行ロードする
     * @param relativeFolder ルートディレクトリからの相対パス
     */
    void PreloadAllUnder(const std::string& relativeFolder);

    /**
     * @brief 現在キャッシュされているモデルのキー（パス）一覧を取得
     * @return キーのリスト
     */
    std::vector<std::string> GetCachedKeys() const;

    /**
     * @brief 参照されなくなったキャッシュエントリーを削除する
     */
    void CollectGarbage();

    /**
     * @brief すべてのキャッシュを破棄する
     */
    void ClearAll();

    // --- ロード関数群 (内部的または特殊用途で使用) ---

    /**
     * @brief 個別のOBJファイルをロードする (複数メッシュ・マテリアル対応)
     */
    static ObjModel LoadObjFileM(const std::string& directoryPath, const std::string& filename);

    /**
     * @brief 汎用モデルファイルをロードする (Assimp使用)
     */
    static ObjModel LoadModelFromFile(const std::string& directoryPath, const std::string& filename);

    /**
     * @brief ヴォクセル化モデルを生成する
     */
    static VoxelizedModel VoxelizeModel(const ObjModel& model, const Vector3Int& resolution, TextureManager* textureManager);

private:
    /**
     * @brief ファイルパスを正規化し、解決する
     */
    std::string NormalizeAndResolve(const std::string& filename) const;

    /**
     * @brief 文字列が特定の接頭辞で始まっているか判定
     */
    static bool StartsWith(const std::string& s, const std::string& prefix);

    /**
     * @brief フルパスをディレクトリとファイル名に分割する
     */
    static std::pair<std::string, std::string> SplitDirectoryAndFile(const std::string& full);

    /**
     * @brief ロード状況のデバッグログ出力
     */
    void DebugLogLoad(const std::string& key, size_t meshCount);

    /**
     * @brief モデルファイルを再帰的に検索してパスを返す
     */
    std::string FindFileRecursive(const std::string& filename) const;

    // --- 旧形式との互換性用もしくは内部ユーティリティ ---
    static bool ParseObjFaceToken(const std::string& token, int& posIdx, int& uvIdx, int& normIdx);
    static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string filename);
    static Node ReadNode(aiNode* node);

    // 以下の古い形式は非推奨または内部管理用に限定
    static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
    static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);

private:
    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    std::string rootDir_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<ManagedModel>> cache_;
    mutable std::unordered_map<std::string, std::string> filePathCache_;
};