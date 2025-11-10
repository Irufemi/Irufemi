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
#include "math/ObjModel.h"
#include "math/ModelData.h"
#include "math/MaterialData.h"
#include "math/Node.h"
#include "function/Math.h"

// 前方宣言（assimp）
struct aiNode;
namespace Assimp { class Importer; }
struct aiScene;
struct aiMesh;
struct aiMaterial;

class ModelManager {
public:
    ModelManager() = default;
    ~ModelManager() = default;

    // ---- インスタンス機能（キャッシュ管理） ----
    void SetRootDirectory(std::string root);

    // 初期化（必要に応じて将来拡張）
    void Initialize();

    // モデル取得（キャッシュにあれば再利用 / なければロード）
    // filename: 相対("sample/bunny.obj") / ルートを含む("resources/obj/sample/bunny.obj") 両対応
    std::shared_ptr<ObjModel> GetModel(const std::string& filename);

    // 事前ロード（フォルダ内再帰走査）
    void PreloadAllUnder(const std::string& relativeFolder);

    // キャッシュ内の有効キー一覧
    std::vector<std::string> GetCachedKeys() const;

    // 弱参照のみになったエントリ削除
    void CollectGarbage();

    // 強制クリア
    void ClearAll();

    // ---- 静的ロード関数 (旧 Function.h 由来) ----
    // 手書き obj → ModelData
    static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
    // 手書き obj → ObjModel (複数 Mesh + Node 無し)
    static ObjModel LoadObjFileM(const std::string& directoryPath, const std::string& filename);
    // Assimp 汎用 → ModelData
    static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);
    // 旧名称互換（必要なら残す）
    static ObjModel LoadObjFileAssimpM(const std::string& directoryPath, const std::string& filename);
    // Assimp 汎用 → ObjModel (Node 階層あり)
    static ObjModel LoadModelFileM(const std::string& directoryPath, const std::string& filename);
    // f 行トークンパース
    static bool ParseObjFaceToken(const std::string& token, int& posIdx, int& uvIdx, int& normIdx);
    // MTL ロード
    static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string filename);
    // Node 再帰構築
    static Node ReadNode(aiNode* node);

private:

    // 正規化＆ルート解決
    std::string NormalizeAndResolve(const std::string& filename) const;

    static bool StartsWith(const std::string& s, const std::string& prefix);
    
    // "resources/obj/aaa/bbb.obj" → ("resources/obj/aaa", "bbb.obj")
    static std::pair<std::string, std::string> SplitDirectoryAndFile(const std::string& full);

    void DebugLogLoad(const std::string& key, size_t meshCount);

private:
    std::string rootDir_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<ObjModel>> cache_;
};