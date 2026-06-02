# IrufemiEngine 取扱説明書 (Manual)

## エディタ画面のレイアウトについて

エディタの画面構成（ドッキングウィンドウの配置など）が崩れてしまった場合や、チーム内で定められた最新の共通レイアウトに更新したい場合は、以下の手順で復元できます。

1. エディター画面上部のメニューバーから **`Window`** をクリック
2. **`Layout` -> `Load Default Layout`** をクリック

現在の自分の使いやすい配置をチームの新しいデフォルト設定にしたい場合は、並び替えたあとに **`Save Current as Default`** を押し、変更された `default_imgui.ini` をGitでコミットしてください。
（※初回クローン時は自動的に共通レイアウトが適用されるようになっています）

---

## パーティクルシステム (GPUParticleSystem) の利用方法

本エンジンのパーティクルシステムは、コンピュートシェーダー(CS)によってGPU上で高速に動作します。
スクリプトやコンポーネントから以下の手順でエミッターを追加・操作することができます。

### コンポーネントからの利用
GameObject に `ParticleEmitterComponent` をアタッチするだけで、自動的に `GPUParticleManager` に登録され、描画が行われます。
設定可能なパラメータはインスペクター上、またはコードから `emitCountPerFrame_` や `lifeTimeMin_` などを変更してください。

### プログラムからの直接利用 (例)
特定の場所でワンショットのパーティクルを発生させたい場合など。

```cpp
#include "Renderer/ParticleGPU/GPUParticleManager.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"

// 1. マネージャーにエミッターを登録（テクスチャパスを指定）
auto handle = GPUParticleManager::GetInstance()->RegisterEmitter("effect/particle_tex.png");

// 2. パラメータを設定してマネージャーに更新を通知
GPUParticleEmitter data;
data.emit = 1;
data.type = 0; // 0: Sphere, 1: Beam, 2: Ring, 3: Cylinder, 4: Box
data.translateX = 10.0f;
data.translateY = 5.0f;
data.translateZ = 0.0f;
data.count = 50;         // 放出量
data.frequency = 0.1f;   // 放出間隔

// 3. データの適用
GPUParticleManager::GetInstance()->UpdateEmitterData(handle, data);

// 4. 使用が終わったら登録解除（自動的に空きスロットとして再利用されます）
// GPUParticleManager::GetInstance()->UnregisterEmitter(handle);
```

### 【NEW】ゲーム中での一時的なエフェクト再生 (爆発など)
シーン内の特定座標に単発（ワンショット）の爆発エフェクトなどを出したい場合は、新しく追加された `Effect` クラスを使用するのが最も簡単です。

```cpp
#include "Framework/Component/Renderer/Effect.h"

// 座標とパーティクルのテクスチャパスを指定してエフェクトを発生
Effect::PlayEffect(
    Vector3(10.0f, 0.0f, 5.0f),       // 発生座標
    "resources/texture/explosion.png",// テクスチャ
    ParticleType::Sphere,             // パーティクルの形状 (爆発なら Sphere や Hemisphere)
    500                               // 発生させるパーティクルの数 (Burst量)
);
```

### インスペクターからの ParticleType などの設定
`ParticleEmitterComponent` を GameObject にアタッチした場合、エディターの **Inspector パネル** から以下の新機能を直感的に操作できます。

- **Particle Mesh & Shape (形状と発生範囲)**
  - `Sphere`, `Beam`, `Ring`, `Cylinder`, `Box` などの発生形状を選択可能です。
  - `Box` を選択した場合のみ、専用の `Area Size (X,Y,Z)` を指定して箱状の範囲内に発生させることができます。
  - **Billboard Mode**: パーティクルのカメラに対する向きを `None` (固定), `Billboard` (常にカメラを向く), `Y-Axis` (Y軸固定でカメラを向く・魔法陣などに最適) から選べます。

- **Animation & Visuals (アニメーションと見た目)**
  - **Atlas Rows / Cols**: 連番テクスチャ（スプライトシート）の分割数を指定するだけで、自動的にアニメーション再生されます。
  - **Start / Mid / End Color & Scale**: これまでの開始/終了だけでなく、「中間色・中間スケール」と「それがどのタイミング(Mid Point)で切り替わるか」を設定でき、爆発（白→オレンジ→黒煙）などの複雑な表現が可能になりました。

- **Physics (物理挙動)**
  - 重力やバウンドに加えて、**Jitter (ジッター)** によって不規則なブレ（ノイズ）を与え、魔法の粉や舞い散る火の粉のようなランダムな動きを表現できます。

これらのパラメータはすべて Inspector のGUIからリアルタイムに変更・確認できます。
