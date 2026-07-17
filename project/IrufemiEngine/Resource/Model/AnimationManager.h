#pragma once

#include <string>
#include "Data/Animation.h"
#include "Data/NodeAnimation.h"
#include "Data/Joint.h"
#include "Data/Node.h"
#include "Data/SkeletonData.h"
#include "Data/SkeletonPose.h"
#include "Data/SkinCluster.h"
#include <optional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <wrl.h>
#include <d3d12.h>
#include "../../Engine/Core/System/DirectoryWatcher.h"

// 前方宣言
class DirectXCommon;
struct ModelData;
struct ObjModel; // 追加

class AnimationManager
{
public:
    AnimationManager() = default;
    ~AnimationManager() = default;

    // --- インスタンス機能 ---
    void Initialize(DirectXCommon* dxCommon);
    void SetRootDirectory(std::string root);
    std::shared_ptr<Animation> LoadAnimationFile(const std::string& filename);
    SkinCluster CreateSkinCluster(const SkeletonData& skeleton, const ObjModel& objModel);

public: // 静的ヘルパ

    /// <summary>
    /// 任意の時刻の値を取得する
    /// </summary>
    /// <param name="keyframes"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    static Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);

    /// <summary>
    /// 任意の時刻の値を取得する
    /// </summary>
    /// <param name="keyframes"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    static Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);

    /// <summary>
    /// 任意の時刻の値を取得する
    /// </summary>
    /// <param name="keyframes"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    static Vector3 CalculateValue(const AnimationCurve<Vector3>& keyframes, float time);

    /// <summary>
    /// 任意の時刻の値を取得する
    /// </summary>
    /// <param name="keyframes"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    static Quaternion CalculateValue(const AnimationCurve<Quaternion>& keyframes, float time);

    /// <summary>
    /// 任意の時刻の値を取得する(オイラー角)
    /// </summary>
    /// <param name="keyframes"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    static Vector3 CalculateValueAsEuler(const AnimationCurve<Quaternion>& keyframes, float time);

    /// <summary>
    /// Nodeの階層構造からSkeletonDataを作る
    /// </summary>
    /// <param name="rootNode"></param>
    /// <returns></returns>
    static SkeletonData CreateSkeletonData(const Node& rootNode);

    /// <summary>
    /// SkeletonDataからインスタンス用のSkeletonPoseを生成する
    /// </summary>
    /// <param name="data"></param>
    /// <returns></returns>
    static SkeletonPose CreateSkeletonPose(const SkeletonData* data);

    /// <summary>
    /// NodeからJointDataを作る
    /// </summary>
    /// <param name="node"></param>
    /// <param name="parent"></param>
    /// <param name="joints"></param>
    /// <returns></returns>
    static int32_t CreateJointData(const Node& node, const std::optional<int32_t>& parent, std::vector<JointData>& joints);

    /// <summary>
    /// SkeletonPoseの更新
    /// </summary>
    /// <param name="skeleton"></param>
    static void SkeletonUpdate(SkeletonPose& skeleton);

    /// <summary>
    /// SkeletonPoseに対してAnimationを適用する
    /// </summary>
    /// <param name="skeleton"></param>
    /// <param name="animation"></param>
    /// <param name="animationTime"></param>
    /// <param name="applyRootTranslation">Rootボーンの移動を適用するかどうか（Root Motion抽出時はfalseにする）</param>
    static void ApplyAnimation(SkeletonPose& skeleton, const Animation& animation, float animationTime, bool applyRootTranslation = true);

    /// <summary>
    /// 2つのAnimationをブレンドしてSkeletonPoseに適用する
    /// </summary>
    /// <param name="skeleton"></param>
    /// <param name="animA"></param>
    /// <param name="timeA"></param>
    /// <param name="animB"></param>
    /// <param name="timeB"></param>
    /// <param name="weight">animBの重み (0.0 ~ 1.0)</param>
    /// <param name="applyRootTranslation"></param>
    static void BlendAnimation(SkeletonPose& skeleton, const Animation& animA, float timeA, const Animation& animB, float timeB, float weight, bool applyRootTranslation = true);

    /// <summary>
    /// SkinClusterの更新
    /// </summary>
    /// <param name="skinCluster"></param>
    /// <param name="skeleton"></param>
    /// <param name="frameIndex"></param>
    static void SkinClusterUpdate(SkinCluster& skinCluster, const SkeletonPose& skeleton, uint32_t frameIndex);

private: // 内部ヘルパ
    std::string NormalizeAndResolve(const std::string& filename) const;
    static bool StartsWith(const std::string& s, const std::string& prefix);
    static std::pair<std::string, std::string> SplitDirectoryAndFile(const std::string& full);
    std::string FindFileRecursive(const std::string& filename) const;

private:
    DirectXCommon* dxCommon_ = nullptr;
    std::string rootDir_;
    mutable std::mutex mutex_;
    
    struct CachedAnimation {
        std::shared_ptr<Animation> animation;
        uint64_t lastLoadTime = 0;
        std::string sourceFilePath;
    };
    std::unordered_map<std::string, CachedAnimation> cache_;
    mutable std::unordered_map<std::string, std::string> filePathCache_;
    
    std::unique_ptr<DirectoryWatcher> directoryWatcher_;
    void OnDirectoryChanged();
};

