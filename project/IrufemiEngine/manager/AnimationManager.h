#pragma once

#include <string>
#include "math/Animation.h"
#include "math/NodeAnimation.h"


class AnimationManager
{
public:
    /// <summary>
    /// Animationを解析する
    /// </summary>
    /// <param name="directoryPath"></param>
    /// <param name="filename"></param>
    /// <returns></returns>
    static Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);

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
};

