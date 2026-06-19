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

## チーム開発ルール・コーディング規約

チームでの共同開発（`Application_team` など）を進めるにあたり、以下のアーキテクチャ・コーディング規約を遵守してください。

### 1. アーキテクチャと関心の分離
- **エンジンの独立性**: `IrufemiEngine/` フォルダ配下のコア機能には、特定のゲームやシーンに依存する処理・固有のデータ・アクターを**絶対に含めない**でください。
- **ゲームロジックの配置**: ゲーム固有のロジックやキャラクター制御は、必ず `Application_team/` (または各ゲームの Application フォルダ) 内に記述し、エンジンとアプリケーションの境界を厳格に保ちます。

### 2. メモリ管理と安全性 (C++ / DirectX)
- **スマートポインタの利用**: メモリリークを防ぐため、生ポインタ(`Raw Pointer`)の新規使用は極力避け、用途に合わせて `std::unique_ptr` や `std::shared_ptr` を優先してください。
- **COMオブジェクト管理**: DirectXのオブジェクトを扱う際は、必ず `Microsoft::WRL::ComPtr` を使用して安全にライフサイクルを管理してください。
- **エラーチェック**: DirectXのAPI呼び出し時は `HRESULT` の戻り値を必ずチェックし、適切なエラーハンドリング（アサート等）を含めてください。

### 3. 命名規則・コードスタイル
- **メンバ変数の命名**: `m_` などの接頭辞は使用せず、**キャメルケースの末尾にアンダーバー**をつけるスタイル (`variableName_`) に統一してください。
- **ヘッダーの注釈**: 関数やクラスのコメントは「Doxygen形式」で記述してください。
- **インクルードガード**: `#pragma once` を使用してください。
- **既存への適応**: 新しくクラスや関数を追加する際は、必ず周囲の「既存のコードベースの命名規則」に合わせ、自己流のスタイルを混入させないでください。
- **文字コードとフォーマット**: ファイルはすべて `UTF-8 (署名なし)` で保存し、`.clang-format` による自動整形を活用してください。
