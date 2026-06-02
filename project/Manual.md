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
