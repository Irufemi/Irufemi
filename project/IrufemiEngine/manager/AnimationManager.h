#pragma once

#include <string>
#include "math/Animation.h"
#include "math/NodeAnimation.h"
#include "math/Skeleton.h"
#include "math/Joint.h"
#include "math/Node.h"
#include <optional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>

class AnimationManager
{
public:
    AnimationManager() = default;
    ~AnimationManager() = default;

    // --- インスタンス機能 ---
    void Initialize();
    void SetRootDirectory(std::string root);
    Animation LoadAnimationFile(const std::string& filename);

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
    /// Nodeの階層構造からSkeletonを作る
    /// </summary>
    /// <param name="rootNode"></param>
    /// <returns></returns>
    static Skeleton CreateSkeleton(const Node& rootNode);

    /// <summary>
    /// NodeからJointを作る
    /// </summary>
    /// <param name="node"></param>
    /// <param name="parent"></param>
    /// <param name="joints"></param>
    /// <returns></returns>
    static int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

    /// <summary>
    /// Skeletonの更新
    /// </summary>
    /// <param name="skeleton"></param>
    static void SkeletonUpdate(Skeleton& skeleton);

    /// <summary>
    /// Skeletonに対してAnimationを適用する
    /// </summary>
    /// <param name="skeleton"></param>
    /// <param name="animation"></param>
    /// <param name="animationTime"></param>
    static void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime);

private: // 内部ヘルパ
    std::string NormalizeAndResolve(const std::string& filename) const;
    static bool StartsWith(const std::string& s, const std::string& prefix);
    static std::pair<std::string, std::string> SplitDirectoryAndFile(const std::string& full);
    std::string FindFileRecursive(const std::string& filename) const;

private:
    std::string rootDir_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::weak_ptr<Animation>> cache_;
    mutable std::unordered_map<std::string, std::string> filePathCache_;
};

