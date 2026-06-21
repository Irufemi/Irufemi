# IrufemiEngine 取扱説明書 (Manual)

## エディタ画面のレイアウトについて

エディタの画面構成（ドッキングウィンドウの配置など）が崩れてしまった場合や、チーム内で定められた最新の共通レイアウトに更新したい場合は、以下の手順で復元できます。

1. エディター画面上部のメニューバーから **`Window`** をクリック
2. **`Layout` -> `Load Default Layout`** をクリック

現在の自分の使いやすい配置をチームの新しいデフォルト設定にしたい場合は、並び替えたあとに **`Save Current as Default`** を押し、変更された `default_imgui.ini` をGitでコミットしてください。
（※初回クローン時は自動的に共通レイアウトが適用されるようになっています）

---

## GameObject とコンポーネントの基本操作

本エンジンは、Unityなどのモダンなエンジンと同様のコンポーネント指向で設計されています。シーン内のオブジェクト（`GameObject`）に対して、様々な機能（`Component`）をアタッチ・取得することでゲームロジックを構築します。

### 基本的なコンポーネントの取得
アタッチされているコンポーネントを取得するには、`GetComponent<T>()` を使用します。
```cpp
// 自身にアタッチされている TransformComponent を取得する
if (auto transform = gameObject_->GetComponent<TransformComponent>()) {
    transform->SetPosition(Vector3(0, 10, 0));
}
```

### GetComponentsInChildren を使った子孫の探索
`GameObject` 自身およびすべての子孫階層から特定のコンポーネントを一括検索する強力な機能が備わっています。特定のオブジェクト（プレハブのルートなど）の下に連なっているパーティクルやコライダーを一斉に操作したい場合に非常に便利です。

```cpp
auto obj = gameObject_->GetScene()->FindGameObject("Player");

// 1. ツリー全体から、指定した型のコンポーネントを「すべて（配列で）」取得する
auto emitters = obj->GetComponentsInChildren<ParticleEmitterComponent>();
for (auto pe : emitters) {
    pe->Restart(false); // 全てのエミッターを一斉に再発火
}

// 2. ツリー全体から、指定した型のコンポーネントを「1つだけ」取得する
if (auto collider = obj->GetComponentInChildren<ColliderComponent>()) {
    collider->SetIsActive(false);
}
```

**【重要】**
この探索機能は「自身（ルート）にアタッチされているコンポーネント」も検索対象に含まれます。そのため、ルートに目的のコンポーネントがあるかどうかを事前に気にする必要はなく、非常にシンプルで堅牢なコードを記述できます。

---

## チーム開発ルール・コーディング規約

チームでの共同開発（`Application_team` など）を進めるにあたり、以下のアーキテクチャ・コーディング規約を遵守してください。

### 1. アーキテクチャと関心の分離
- **エンジンの独立性**: `IrufemiEngine/` フォルダ配下のコア機能には、特定のゲームやシーンに依存する処理・固有のデータ・アクターを**絶対に含めない**でください。
- **ゲームロジックの配置**: ゲーム固有のロジックやキャラクター制御は、必ず `Application_team/` (または各ゲームの Application フォルダ) 内に記述し、エンジンとアプリケーションの境界を厳格に保ちます。

### 2. メモリ管理と安全性 (C++ / DirectX)
- **スマートポインタの利用**: メモリリークを防ぐため、生ポインタ(`Raw Pointer`)の新規使用は極力避け、用途に合わせて `std::unique_ptr` や `std::shared_ptr` を優先してください。
- **COMオブジェクト管理**: DirectXのオブジェクトを扱う際は、必ず `Microsoft::WRL::ComPtr` を使用して安全にライフサイクルを管理してください。
- **エラーチェック**: DirectXのAPI呼び出し時は `HRESULT` の戻り値を必ずチェックし、適切なエラーハンドリング（アサート等）を含めてください。

### 3. オブジェクトのライフサイクル管理とプールの安全な運用
シューティングゲームの敵やヒットエフェクトなど、頻繁に生成と消滅を繰り返すオブジェクト（プーリング対象）を実装する際は、メモリ破壊やプールの崩壊を防ぐために以下の**厳密なルール**に従う必要があります。

- **プール運用オブジェクトでは絶対に `Destroy()` を呼ばない**
  `Destroy()` を呼ぶと `GameObject` がシーンから完全に削除（メモリ解放）されてしまいます。プール運用しているオブジェクトが削除されると、プール側にダングリングポインタが残り、次回取り出した際にクラッシュします。
  **プーリング対象のオブジェクトが死ぬときは、必ず「非アクティブ化 (SetIsActive(false))」してプールへ返却（Release）してください。**
  ```cpp
  enemyComp->SetOnDeathCallback([this](GameObject* deadObj) {
      deadObj->SetIsActive(false);
      if (enemyPool_) {
          enemyPool_->Release(deadObj->shared_from_this());
      }
  });
  ```

- **LifetimeComponent の TimeoutAction を活用する**
  一定時間で消滅するエフェクトなどに `LifetimeComponent` をアタッチする場合、インスペクタ上で **Timeout Action** を変更できます。単発生成なら `Destroy` (デフォルト)、プール運用なら `Disable` を設定してください。コードから生成する際は以下のように上書きすると安全です。
  ```cpp
  auto obj = gameObject_->Instantiate(effectPrefabPath);
  if (auto lifetime = obj->GetComponent<LifetimeComponent>()) {
      lifetime->SetTimeoutAction(TimeoutAction::Disable);
  }
  ```

### 4. GPUリソースの事前確保（Pre-warming）によるラグ防止
リアルタイム性が命のゲームにおいて、実行中のテクスチャ読み込みやシェーダー・VRAMバッファの構築（遅延評価）は**画面のカクつき（Hitch / Stutter）を生む最大の原因**となります。一線級のエンジンと同様に、ゲーム開始前の初期化フェーズ（ロード画面や `Start()` のタイミング）で重い処理を強制的に終わらせる**プレウォーム（事前確保）**を徹底してください。

- **安全なプレウォームの実装方法**
  初期化時にフルセットの更新処理を呼ぶと、プールの中で休眠中のエフェクトが勝手に発生・消費されてしまう（いざ使うときに出なくなる）バグの原因になります。これを防ぐため、**「発生パラメータは一切送らず、GPUマネージャーに対してハンドルの取得（VRAM領域の予約）だけを行う」専用のメソッド**を用意・使用してください。
  ```cpp
  // ParticleObject.cpp の例
  void ParticleObject::Initialize() {
      // ...
      // 実行中のラグを防ぐため、安全なプレウォーム（枠だけの事前確保）を行う
      PrewarmSystem();
  }

  void ParticleObject::PrewarmSystem() {
      // パラメータは送信せず、マネージャーに「このテクスチャとブレンドモードを使う」と登録だけ行う
      if (!emitterHandle_.IsValid() && gpuParticleManager_) {
          emitterHandle_ = gpuParticleManager_->RegisterEmitter(texturePath_, blendMode_, isUnscaledTime_);
      }
  }
  ```

### 5. 命名規則・コードスタイル
- **メンバ変数の命名**: `m_` などの接頭辞は使用せず、**キャメルケースの末尾にアンダーバー**をつけるスタイル (`variableName_`) に統一してください。
- **ヘッダーの注釈**: 関数やクラスのコメントは「Doxygen形式」で記述してください。
- **インクルードガード**: `#pragma once` を使用してください。
- **既存への適応**: 新しくクラスや関数を追加する際は、必ず周囲の「既存のコードベースの命名規則」に合わせ、自己流のスタイルを混入させないでください。
- **文字コードとフォーマット**: ファイルはすべて `UTF-8 (署名なし)` で保存し、`.clang-format` による自動整形を活用してください。

---

## パーティクルシステム (GPUParticleSystem) の利用方法

本エンジンのパーティクルシステムは、コンピュートシェーダー(CS)によってGPU上で高速に動作します。
スクリプトやコンポーネントから以下の手順でエミッターを追加・操作することができます。

### コンポーネントからの利用
GameObject に `ParticleEmitterComponent` をアタッチするだけで、自動的にエディタ上で操作・プレビューが可能です。
エディタ（ImGui）上で設定したパラメータは、JSONファイルとして自動的にシリアライズされ、再実行時にも完全に復元されます。

### プログラムからの直接利用 (ParticleObject)
ゲーム内でコードから動的にパーティクルを生成・制御したい場合は `ParticleObject` クラスを使用します。
JSONファイルから設定をロードすることで、エディタで作成した複雑なエフェクトをそのまま呼び出すことができます。

```cpp
#include "Renderer/ParticleGPU/ParticleObject.h"

// 1. ParticleObject の生成とJSONの読み込み
ParticleObject myParticle;
myParticle.LoadFromJson("resources/particles/explosion.json");

// 2. 座標や必要に応じたパラメータの上書き
myParticle.position_ = Vector3(10.0f, 5.0f, 0.0f);
myParticle.emissionRate_ = 100.0f; // 1秒間に100個発生

// （パラメータをコードから変更した場合は MarkDirty() を呼ぶか、Update内で自動反映されます）
myParticle.MarkDirty();

// 3. 毎フレーム Update を呼ぶ
myParticle.Update();

// 4. 一度に大量に発生（バースト）させたい場合
myParticle.EmitBurst(50);
```

### 【上級者向け】GPUParticleManager の直接利用
直接マネージャーに通信して描画リクエストを送ることも可能です（独自の最適化を行いたい場合など）。
※現在のマネージャーは「テクスチャ + ブレンドモード + タイムスケール」の複合キーで管理されています。

```cpp
#include "Renderer/ParticleGPU/GPUParticleManager.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"

// 1. マネージャーにエミッターを登録（テクスチャ、ブレンドモード、ポーズ中動作フラグ）
auto handle = GPUParticleManager::GetInstance()->RegisterEmitter(
    "effect/particle_tex.png", 
    BlendMode::kBlendModeAdd, 
    false // trueにするとポーズ中(UnscaledTime)でも動作する
);

// 2. パラメータを設定してマネージャーに更新を通知
GPUParticleEmitter data;
data.emit = 1;
data.type = 0; // 0: Sphere, 1: Beam, 2: Ring, 3: Cylinder, 4: Box
data.translateX = 10.0f;
data.emissionRate = 50.0f; // 1秒あたりの連続放出数

// 3. データの適用
GPUParticleManager::GetInstance()->UpdateEmitterData(handle, data);
```

### 【NEW】ゲーム中での一時的なエフェクト再生 (爆発など)
シーン内の特定座標に単発（ワンショット）の爆発エフェクトなどを出したい場合は、新しく追加された `Effect` クラスを使用するのが最も簡単です。

```cpp
#include "Renderer/Effect/Effect.h"

// 1. エフェクトインスタンスの作成と初期化（例：爆発）
Effect myEffect;
myEffect.Initialize(EffectType::kExplosion);

// 2. 指定した座標でエフェクトを発生させる
myEffect.Play(Vector3(10.0f, 0.0f, 5.0f));

// 3. 毎フレーム Update と Draw を呼ぶ
myEffect.Update();
myEffect.SyncBeforeDraw();
myEffect.Draw();
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

【重要】これらのパラメータはすべて Inspector のGUIからリアルタイムに変更・確認できます。
本エンジンでは `ParticleEmitterComponent` が `CanUpdateInEditMode()` をサポートしているため、**ゲームを再生していなくても（エディタ編集モードでも）、パラメータを変更した瞬間にリアルタイムでパーティクルの見た目が更新・プレビューされます。**

### 【開発者向け】パラメータ追加・UI連携の仕組み (RegisterProperties)
`ParticleObject` が持つパラメータ群をインスペクターに表示するためのUI登録処理は、すべて `ParticleObject::RegisterProperties(Component* comp)` というメソッドに集約されています。
これにより、エンジンコアの描画処理とエディターUIの責務が完全に分離されました。もし新しい機能やパラメータを追加したい場合は、この `RegisterProperties` 内に `comp->RegisterProperty(...)` などを1行追記するだけで、自動的にエディタUIに項目が追加され、保存（シリアライズ）も連動して行われるようになっています。

---

## ポストプロセス (PostProcessManager) の利用方法と描画順序

画面全体にかけるポストプロセスエフェクト（PostProcessManager）を使用する際は、**「エフェクトをスタックに追加する順番（描画順序）」** を意識することで、プロの現場でも通用する意図した映像表現が可能になります。

### 推奨される描画順序（スタックに追加する順）
1. **色調補正系**: ToneMapping, Grayscale, Sepia, HSV など
2. **空間・ぼかし系**: Smoothing, GaussianFilter, RadialBlur など
3. **画面演出系**: Vignette, Noise, Glitch, Dissolve など
4. **画面遷移系**: Fade, Slide など

**なぜこの順番なのか？**
例えば、`Vignette`（画面の端を暗くする/色をつける演出）のあとに `Grayscale`（白黒化）をかけてしまうと、ビネットで赤色などを指定してもモノクロになってしまいます。「色調補正」を先に行い、その上から「画面演出」を乗せるのがセオリーです。

### 【NEW】Vignetteのパラメータ変更について
Vignetteエフェクトがより自然な減衰（Smooth Falloff）になるようパラメータがアップグレードされました。
- **`radius` (旧: scale)**: 減衰が始まる半径 (デフォルト 0.8)
- **`softness` (旧: power)**: 減衰の柔らかさ (デフォルト 0.5)

これにより、画面端が完全に黒く潰れるのを防ぎ、滑らかなグラデーション表現が可能になっています。シーン初期化時などでパラメータを調整する際はご留意ください。

---

## Audio システム (AudioPlayer / AudioSourceComponent) の利用方法

本エンジンのAudioシステムは、コンポーネントからの利用とプログラムからの直接利用の2通りの方法をサポートしています。
これまでの `Bgm` や `Se` クラスは廃止され、統合された `AudioPlayer` クラスによって一元管理されます。リソースリークを防ぐ安全な設計（ComPtrやRAIIの活用）が内部で行われているため、プログラマは生成・破棄のタイミングを気にせず利用できます。

### コンポーネントからの利用
GameObject に `AudioSourceComponent` をアタッチすることで、インスペクタ（エディタ）上からサウンドの設定が可能です。
- **Audio Type**: `BGM` か `SE` かを選択します。BGMはデフォルトでループ再生されます。
- **File Path**: 再生するオーディオファイルのパス（例: `resources/audio/bgm/field.wav`）を指定します。
- **Volume**: 音量を 0.0 ～ 1.0 の間で調整します。
- **Loop**: ループ再生のON/OFFを任意に切り替えます。

スクリプトから再生・停止を行う場合は、コンポーネントを取得して以下のように呼び出します。
```cpp
auto audioSource = GetComponent<AudioSourceComponent>();
if (audioSource) {
    audioSource->Play(); // 再生（インスペクタの設定が反映されます）
    // audioSource->Stop(); // 停止したい場合
}
```

### プログラムからの直接利用 (AudioPlayer)
UIの操作音や、特定のコンポーネント（GameObject）に紐付かない効果音を再生する場合は、直接 `AudioPlayer` クラスのインスタンスを生成して使用するのが便利です。

```cpp
#include "Resource/Audio/AudioPlayer.h"
#include "Resource/Audio/AudioType.h"

// 1. エンジンから AudioManager を取得して、AudioPlayer を生成
auto audioManager = engine->GetAudioManager();
AudioPlayer clickSe(audioManager, AudioType::SE);

// 2. 音声ファイルのパスを指定して初期化
clickSe.Initialize("resources/audio/se/click.wav");

// 3. 再生（オプションで音量設定などが可能）
clickSe.SetVolume(0.8f);
clickSe.Play(); // 1回再生

// 再生中のサウンドを明示的に止めたい場合は Stop() を呼びます
// clickSe.Stop();
```

---

## アクションベース入力システム (Input Action System) の利用方法

現在の入力システムは、キーボードやゲームパッドのボタンを直接監視するのではなく、物理入力と「論理アクション（例：Jump、Move）」を紐付けて管理する「アクションベース」へ移行しています。これにより、ユーザーのキーコンフィグの変更や複数デバイスの同時対応が容易になります。

### 使い方（初期化時）
ゲームの初期化処理（Sceneの `Initialize` など）で、`InputManager` に対してアクションと物理デバイス（キーやボタン）の対応付け（バインディング）を行います。

```cpp
InputManager* input = engine_->GetInputManager();

// "Jump" アクションに Spaceキー と ゲームパッドのAボタン を割り当て
input->BindAction("Jump", InputId::Keyboard_Space);
input->BindAction("Jump", InputId::GamePad_A);

// "MoveX" アクション（アナログ/1D軸入力）の割り当て
// スケール値（第3引数）を使って、物理入力を最終的なアクション値に変換します。
// 例: Dキー(1.0)は右方向(+1.0)に、Aキー(1.0)はマイナスを掛けて左方向(-1.0)にする
input->BindAction("MoveX", InputId::Keyboard_D, 1.0f);
input->BindAction("MoveX", InputId::Keyboard_A, -1.0f);
// パッドのスティックはそのままの値(-1.0～1.0)を使う、あるいは 0.5f 等を掛けて感度調整も可能
input->BindAction("MoveX", InputId::GamePad_LeftStickX, 1.0f);
```

### 使い方（更新時）
毎フレームの更新処理（Sceneの `Update` など）では、バインドしたアクション名を指定して状態を取得します。

```cpp
InputManager* input = engine_->GetInputManager();

// デジタル入力（ボタンが押された瞬間）の判定
if (input->IsActionTriggered("Jump")) {
    // ジャンプ処理を実行
}

// アナログ入力（移動量など）の取得
float moveInputX = input->GetActionValue("MoveX").x;
// プレイヤーの移動処理へ
```

※ 互換性維持のため、従来の `IsKeyDown(VK_SPACE)` などのAPIも引き続き使用可能ですが、新しくコードを書く際はアクションシステム (`BindAction`, `GetActionValue` 等) を積極的に利用することが推奨されます。

---

## 【NEW】GPU カリング (GPU Culling & ExecuteIndirect) の利用方法

本エンジンでは、大量の同一モデル（がれき、草、パーティクルなど）を描画する際のCPU負荷（フラスタムカリングやメモリ転送）を劇的に削減するため、**Compute Shader による GPU フラスタムカリング** をサポートしました。

この機能を有効にすると、CPU側でのカリング判定がスキップされ、GPUが自身で「カメラに映っているオブジェクト」だけを判定し、`ExecuteIndirect` を通して一括描画するようになります。

### 利用方法 (コンポーネントからの利用)
最も簡単な方法は、`ModelBatchRendererComponent` （またはそれを内部で生成するマネージャークラス）に対して、初期化時にフラグを有効化することです。

```cpp
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"

// 1. バッチレンダラーコンポーネントの取得または追加
auto batchRenderer = gameObject->AddComponent<ModelBatchRendererComponent>();

// 2. GPUカリングを有効にする
batchRenderer->SetUseGPUCulling(true);
```

### 利用方法 (プログラム/バッチからの直接利用)
`ModelBatch` などのバッチクラスを直接生成して描画している場合も、同様に `SetUseGPUCulling(true)` を呼び出すだけです。

```cpp
#include "Renderer/Object/Batch/ModelBatch.h"

// 1. バッチの生成
ModelBatch myBatch;
myBatch.Initialize("DebrisModel");

// 2. GPUカリングを有効化
myBatch.SetUseGPUCulling(true);

// 3. インスタンスの追加 (CPU側でTransformを設定)
for (int i = 0; i < 10000; ++i) {
    myBatch.AddInstanceWorld(Matrix4x4::MakeTranslation(Vector3(i * 1.0f, 0, 0)));
}

// 4. 描画
// 内部で自動的にGPUカリング用Compute Shaderが実行され、その後ExecuteIndirectで描画されます
myBatch.Draw();
```

### 注意点・制限事項
- GPUカリングは、**1000個以上の大量のインスタンス**を描画する場合に効果を発揮します。少数のオブジェクトに対しては、逆にCompute Shaderのディスパッチオーバーヘッドが上回る可能性があります。
- 描画対象のオブジェクトには、ローカル空間での正確な **バウンディングスフィア (BoundingSphere)** が設定されている必要があります（`GetBoundingSphereRadius` の値がカリングに使用されます）。

---

## 【NEW】大量オブジェクトの最適化 (VirtualEntityManagerComponent) の利用方法

本エンジンでは、数万個レベルの大量のオブジェクト（がれき、草、弾幕など）を最適化して描画・管理するためのシステムとして、**`VirtualEntityManagerComponent`** (Instance Replacement / Promotion パターン) をサポートしました。

このコンポーネントを使用すると、普段は `GameObject` を実体化せずに軽量な「行列データ（Virtual Transform）」としてのみ管理・GPU描画し、プレイヤーが干渉した瞬間など **本当に必要なときだけ本物のアクター（GameObject）に昇格（Promote）させる** ことができます。

### 導入手順
1. **マネージャーの準備**:
   ゲーム側の管理クラス（例: `DebrisManagerComponent`）で、`VirtualEntityManagerComponent` をアタッチして初期化します。

```cpp
#include "Framework/Component/VirtualEntity/VirtualEntityManagerComponent.h"

// 1. コンポーネントの追加
auto virtualManager = gameObject_->AddComponent<VirtualEntityManagerComponent>();

// 2. プールサイズと生成ファクトリの登録
auto factory = [this]() -> std::shared_ptr<GameObject> {
    auto obj = std::make_shared<GameObject>("MyEntity");
    obj->AddComponent<DebrisComponent>(); // 物理挙動などをアタッチ
    obj->SetIsActive(false);
    gameObject_->AddChild(obj);
    return obj;
};
virtualManager->Setup(500, factory); // 最大500個までは同時に実体化可能
```

2. **データの追加（実体化しない）**:
```cpp
// 座標や回転だけを登録し、仮想インスタンスIDを受け取る
int id = virtualManager->AddVirtualInstance(Vector3(10, 0, 0));
```

3. **データの更新 (Data-Oriented Update)**:
   アニメーションなどを適用したい場合は、`GetVirtualInstances()` で配列を取得し、毎フレーム直接書き換えます。実体がないため超高速に処理されます。
```cpp
auto& instances = virtualManager->GetVirtualInstances();
for (auto& vi : instances) {
    if (!vi.isPromoted_ && !vi.isDestroyed_) {
        // 例：Y座標をフワフワさせる
        vi.position_.y += std::sin(time) * 0.1f;
    }
}
```

4. **昇格 (Promote) と 降格 (Demote)**:
   プレイヤーが近づいた、攻撃を当てた等のタイミングで、IDを指定して本物の `GameObject` に昇格させます。
```cpp
// id を指定してGameObjectをプールから取得し、仮想インスタンスの座標を同期する
auto realObj = virtualManager->Promote(id);
if (realObj) {
    // 物理演算を有効化したり、ターゲットを追従させたりする
}

// 用済みになったらデータに戻す
virtualManager->Demote(id);
```

このシステムと前述の「GPU Culling」を組み合わせることで、数万個のオブジェクトがあっても 60FPS を余裕で維持できるパフォーマンスを実現できます。

---

## 【NEW】TransformComponent の使い方 (カプセル化と遅延評価)

本エンジンでは、描画・物理・ゲームロジック間のキャッシュ効率および同期安全性を高めるため、**`TransformComponent` のアーキテクチャがカプセル化（Data-Oriented Design対応）されました。**

従来のように `transform->position_` のようなパブリックメンバへの直接代入・参照は禁止されており、代わりに **Getter / Setter** を使用する必要があります。

### 主なAPI
```cpp
auto transform = gameObject_->GetComponent<TransformComponent>();

// --- 取得 (Getter) ---
// ローカル座標系
Vector3 localPos = transform->GetPosition();
Vector3 localRot = transform->GetRotation(); // オイラー角(ラジアン)
Vector3 localScl = transform->GetScale();

// ワールド座標系（親のTransformを加味した最終結果）
Vector3 worldPos = transform->GetWorldPosition();
Vector3 worldRot = transform->GetWorldRotation();
Vector3 worldScl = transform->GetWorldScale();

// --- 更新 (Setter) ---
// 値を更新すると、内部で Dirty フラグ (isLocalDirty_) が立ちます
transform->SetPosition(Vector3(10, 5, 0));
transform->SetRotation(Vector3(0, Math::ToRadian(90), 0));
transform->SetScale(Vector3(2, 2, 2));
```

### 【重要】Dirty フラグと遅延評価 (Lazy Evaluation) の仕組み
Setter を通じて座標を変更しても、**その瞬間にすべての行列計算が行われるわけではありません。**
内部では「Dirty（変更あり）」というフラグだけが立ち、実際に `GetWorldMatrix()` や描画処理から行列が要求されたタイミングで、**1回だけ（キャッシュとして）再計算** される遅延評価の仕組みが導入されています。

これにより、同じフレーム内で何度座標を変更しても、無駄な行列計算（sin/cosや行列乗算）が走らないため、非常に高速に動作します。
※ 特別な理由がない限り、自分で `ComputeMatrix()` を呼び出す必要はありません。エンジン側の `BaseScene::Update()` の直後に一括で最新化されます。

---

## 【NEW】半透明・エフェクトオブジェクトの描画とZソート

本エンジンでは、地形やキャラクターなどの不透明オブジェクトの後に、オーラやレーザーなどの半透明エフェクトを正しい順番（奥から手前）で描画するための **独立した半透明描画パス (MainTransparentPass)** をサポートしました。

これまで半透明オブジェクトを描画する際、Zバッファへの書き込み（DepthWrite）をDisableにすると、後から描画される不透明オブジェクトに上書きされて見えなくなる問題がありましたが、この機能を利用することで正しく描画されます。

### 利用方法

半透明や加算合成で描画したい Primitive3DObject （またはそれを保持するRendererComponent）に対して、初期化時に SetIsTransparent(true) を設定し、同時にPSO設定で DepthWrite::Disable を指定します。

```cpp
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"

// 1. オブジェクトの初期化
auto aura = std::make_shared<Primitive3DObject>();
aura->Initialize(PrimitiveType::Sphere);

// 2. カスタムPSOの適用 (DepthWrite を Disable にする)
auto pso = engine->GetPSOManager()->GetPSO("EnergyCore", BlendMode::kBlendModePremultiplied, PSOManager::DepthWrite::Disable, PSOManager::CullMode::Back);
aura->SetCustomPSO(pso);

// 3. 半透明フラグを有効にする (重要！)
// これにより、不透明オブジェクトをすべて描き終わった後に、カメラからの距離でソートされて描画されます。
aura->SetIsTransparent(true);
```

### 注意点
- SetIsTransparent(true) を設定したオブジェクトは、自動的にカメラからの距離（distanceToCamera）を計算し、**Z値の降順（Back-to-Front）**でソートされて描画されます。
- 不透明な通常のモデルに SetIsTransparent(true) を設定しないでください（Early-Zカリングなどの恩恵が受けられず、パフォーマンスが低下します）。
