#include "contents/Quaternion.h"
#include <cmath>

// ---------- クォータニオンのユーティリティ ----------

/// <summary>
/// クォータニオンの正規化
/// </summary>
/// <param name="q">クォータニオン</param>
/// <returns>長さが1に正規化されたクォータニオン</returns>
Quaternion Normalize(const Quaternion &q) {

  float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;

  // 長さが0なら単位クォータニオンを返す
  if (lenSq <= 0.0f) {
    return {0.0f, 0.0f, 0.0f, 1.0f};
  }

  // 正規化
  float invLen = 1.0f / std::sqrt(lenSq);

  return {q.x * invLen, q.y * invLen, q.z * invLen, q.w * invLen};
}

/// <summary>
/// クォータニオンの乗算関数,
/// q = a * b,
/// b 回転した後に a 回転する
/// </summary>
/// <param name="a"></param>
/// <param name="b"></param>
/// <returns> a * b </returns>
Quaternion Multiply(const Quaternion &a, const Quaternion &b) {
  Quaternion r;
  r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
  r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
  r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
  r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
  return r;
}

/// <summary>
/// 任意の軸の回転からクォータニオンを生成
/// </summary>
/// <param name="axis"></param>
/// <param name="angle"></param>
/// <returns></returns>
Quaternion FromAxisAngle(const Vector3 &axis, float angle) {
  float half = angle * 0.5f;
  float s = std::sin(half);
  return {axis.x * s, axis.y * s, axis.z * s, std::cos(half)};
}

/// <summary>
/// クォータニオンをオイラー角に変換する
/// </summary>
/// <param name="qIn"></param>
/// <returns> roll(X), pitch(Y), yaw(Z)の順番で出る </returns>
Vector3 QuaternionToEuler(const Quaternion &qIn) {
  Quaternion q = Normalize(qIn);

  // roll (X軸まわり)
  float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
  float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
  float roll = std::atan2(sinr_cosp, cosr_cosp);

  // pitch (Y軸まわり)
  float sinp = 2.0f * (q.w * q.y - q.z * q.x);
  float pitch;

  // 範囲外なら±90度にクリップする
  if (std::fabs(sinp) >= 1.0f) {
    pitch = std::copysign(3.14159265f / 2.0f, sinp);
  } else {
    pitch = std::asin(sinp);
  }

  // yaw (Z軸まわり)
  float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
  float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
  float yaw = std::atan2(siny_cosp, cosy_cosp);

  return {roll, pitch, yaw};
}

/// <summary>
/// ベクトルをクォータニオンで回す
/// </summary>
/// <param name="v"></param>
/// <param name="q"></param>
/// <returns></returns>
Vector3 RotateByQuaternion(const Vector3 &v, const Quaternion &q) {
  Quaternion qv{v.x, v.y, v.z, 0.0f};
  Quaternion qInv{-q.x, -q.y, -q.z, q.w};

  Quaternion t = Multiply(q, qv);
  Quaternion r = Multiply(t, qInv);

  return {r.x, r.y, r.z};
}
