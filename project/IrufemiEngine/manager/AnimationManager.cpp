#include "AnimationManager.h"

#include "math/Animation.h"
#include "function/Ease.h"

#include <cassert>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

/*Animation*/

///Animationを解析する

Animation AnimationManager::LoadAnimationFile(const std::string& directoryPath, const std::string& filename) {

    // まずはAnimationの長さを秒に変換する
    // ・ｍTicksPerSecond：周波数
    // ・mDuration：ｍTicksPerSecondで指定された周波数における長さ
    // たとえばｍTicksPerSecondが1000というのは、1000Hzのことなので、1Tick(周期)は1msである
    // このとき、mDurationが2000なら、2000ms = 2s である

    Animation animation; // 今回作るアニメーション
    Assimp::Importer importer;
    std::string filePath = directoryPath + "/" + filename;
    const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
    assert(scene->mNumAnimations != 0); // アニメーションがない
    aiAnimation* animationAssimp = scene->mAnimations[0]; // 最初のアニメーションだけ採用。もちろん複数対応するに越したことはない
    animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond); // 時間の単位を秒に変換

    /// NodeAnimationを解析する

    // assimpでは個々のNodeのAnimationをchannelと呼んでいるのでchannelを回してNodeAnimationの情報をとってくる
    for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
        aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
        NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
            aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
            KeyframeVector3 keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // ここも秒に変換
            keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };//右手->左手
            nodeAnimation.translate.keyframes.push_back(keyframe);
        }

        // RotateはmNumRotationKeys/mRotationKeys、ScaleはmNumScalingKeys/mScalingKeysで取得できるので同様に行う。
        // RotateはQuaternionで、右手->左手に変換するために、yとzを反転させる必要がある。Scaleはそのままで良い。
        // keyframe.value = {rotate.x, -rotate.y, -rotate.z, rotate.w};

        // Rotation キーフレームを追加
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
            aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
            KeyframeQuaternion keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 秒に変換
            aiQuaternion& q = keyAssimp.mValue;
            // 右手系->左手系変換: y,z を反転
            keyframe.value = { q.x, -q.y, -q.z, q.w };
            nodeAnimation.rotate.keyframes.push_back(keyframe);
        }

        // Scale キーフレームを追加
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
            aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
            KeyframeVector3 keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond); // 秒に変換
            keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z }; // スケールはそのまま
            nodeAnimation.scale.keyframes.push_back(keyframe);
        }
    }
    // 解析完了
    return animation;
}

// 任意の時刻の値を取得する
Vector3 AnimationManager::CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time) {

    // まずは関数の先頭で特殊なケースを除外しておく
    assert(!keyframes.empty()); // キーがないものは返す値がわからないのでダメ
    if (keyframes.size() == 1 || time <= keyframes[0].time) { // キーが1つか、時刻がキーフレーム前なら最初の値とする
        return keyframes[0].value;
    }

    // 前提としてkeyframesは先頭から時刻の早い順に並んでいる
    // 先頭から順番に時刻を調べ、指定した時刻が範囲内であれば、補間を行い値を返す
    // 補間方法はVector3は線形補間、Quaternionは球面線形補間にする
    for (size_t index = 0; index < keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;
        // indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
        if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
            // 範囲内を補間する
            float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
            return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
        }
    }
    // ここまできた場合は一番後の時刻よりも後ろなので最後の値を返すことにする
    return (*keyframes.rbegin()).value;
}

// 任意の時刻の値を取得する
Quaternion AnimationManager::CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time) {

    // まずは関数の先頭で特殊なケースを除外しておく
    assert(!keyframes.empty()); // キーがないものは返す値がわからないのでダメ
    if (keyframes.size() == 1 || time <= keyframes[0].time) { // キーが1つか、時刻がキーフレーム前なら最初の値とする
        return keyframes[0].value;
    }

    // 前提としてkeyframesは先頭から時刻の早い順に並んでいる
    // 先頭から順番に時刻を調べ、指定した時刻が範囲内であれば、補間を行い値を返す
    // 補間方法はVector3は線形補間、Quaternionは球面線形補間にする
    for (size_t index = 0; index < keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;
        // indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
        if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
            // 範囲内を補間する
            float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
            return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
        }
    }
    // ここまできた場合は一番後の時刻よりも後ろなので最後の値を返すことにする
    return (*keyframes.rbegin()).value;
}

// 任意の時刻の値を取得する
Vector3 AnimationManager::CalculateValue(const AnimationCurve<Vector3>& keyframes, float time) {

    // まずは関数の先頭で特殊なケースを除外しておく
    assert(!keyframes.keyframes.empty()); // キーがないものは返す値がわからないのでダメ
    if (keyframes.keyframes.size() == 1 || time <= keyframes.keyframes[0].time) { // キーが1つか、時刻がキーフレーム前なら最初の値とする
        return keyframes.keyframes[0].value;
    }

    // 前提としてkeyframes.keyframesは先頭から時刻の早い順に並んでいる
    // 先頭から順番に時刻を調べ、指定した時刻が範囲内であれば、補間を行い値を返す
    // 補間方法はVector3は線形補間、Quaternionは球面線形補間にする
    for (size_t index = 0; index < keyframes.keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;
        // indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
        if (keyframes.keyframes[index].time <= time && time <= keyframes.keyframes[nextIndex].time) {
            // 範囲内を補間する
            float t = (time - keyframes.keyframes[index].time) / (keyframes.keyframes[nextIndex].time - keyframes.keyframes[index].time);
            return Lerp(keyframes.keyframes[index].value, keyframes.keyframes[nextIndex].value, t);
        }
    }
    // ここまできた場合は一番後の時刻よりも後ろなので最後の値を返すことにする
    return (*keyframes.keyframes.rbegin()).value;
}

// 任意の時刻の値を取得する
Quaternion AnimationManager::CalculateValue(const AnimationCurve<Quaternion>& keyframes, float time) {

    // まずは関数の先頭で特殊なケースを除外しておく
    assert(!keyframes.keyframes.empty()); // キーがないものは返す値がわからないのでダメ
    if (keyframes.keyframes.size() == 1 || time <= keyframes.keyframes[0].time) { // キーが1つか、時刻がキーフレーム前なら最初の値とする
        return keyframes.keyframes[0].value;
    }

    // 前提としてkeyframes.keyframesは先頭から時刻の早い順に並んでいる
    // 先頭から順番に時刻を調べ、指定した時刻が範囲内であれば、補間を行い値を返す
    // 補間方法はVector3は線形補間、Quaternionは球面線形補間にする
    for (size_t index = 0; index < keyframes.keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;
        // indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
        if (keyframes.keyframes[index].time <= time && time <= keyframes.keyframes[nextIndex].time) {
            // 範囲内を補間する
            float t = (time - keyframes.keyframes[index].time) / (keyframes.keyframes[nextIndex].time - keyframes.keyframes[index].time);
            return Lerp(keyframes.keyframes[index].value, keyframes.keyframes[nextIndex].value, t);
        }
    }
    // ここまできた場合は一番後の時刻よりも後ろなので最後の値を返すことにする
    return (*keyframes.keyframes.rbegin()).value;
}