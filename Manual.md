# IrufemiEngine 取扱説明書 (Manual)

このドキュメントは、IrufemiEngineを利用してゲームを開発するチームメンバーのための総合マニュアルです。
各機能の役割と、すぐに使えるコードスニペット（コピペ用コード）をまとめています。

---

## 0. プロジェクト構造とコーディングルール (Project Structure)

新しくコードを書いたり、リソースを追加する際は以下の配置ルールに必ず従ってください。

- **`IrufemiEngine/` (エンジンコア)**
  - 描画パイプラインや汎用的なマネージャーが置かれます。**ゲーム特有のロジックやアクターは絶対にここに書かないでください。**
- **`Application/` (ゲームロジック)**
  - プレイヤーの動き、敵のAI、各種シーン（Title, InGame等）の処理はすべてここに作成します。
- **`resources/` (リソースデータ)**
  - 3Dモデル（`.obj`, `.gltf`）やテクスチャ（`.png`）、音声（`.wav`）は必ずこのフォルダ以下の適切なディレクトリ（`model/`, `ui/`, `audio/` 等）に配置してください。

---

## 1. エンジンの基本アーキテクチャ (Core Architecture)

### 1.1 IrufemiEngine クラス
エンジン全体を統括するコアクラスです。`WinApp`（ウィンドウ管理）や `DirectXCommon`（DirectX12初期化）を保持し、メインループ（`Update` と `Draw`）を回します。ゲームアプリケーション全体で1つのインスタンスのみが存在します。

### 1.2 SceneManager と IScene (シーン管理)
ゲームの画面（タイトル、インゲーム、リザルトなど）を切り替えるための仕組みです。
新しいシーンを追加する場合は `IScene` または `BaseScene` を継承したクラスを作成します。

#### シーンの作り方
```cpp
#pragma once
#include "Framework/BaseScene.h"
#include <memory>
class ObjClass;

class ExampleScene : public BaseScene {
public:
    ~ExampleScene() override = default;
    
    // シーン遷移時に一度だけ呼ばれる（リソース読み込みなど）
    void Initialize(IrufemiEngine* engine) override;
    
    // 毎フレーム呼ばれる（ロジックの更新）
    void Update() override;
    
    // 毎フレーム呼ばれる（描画リクエストの送信）
    void Draw() override;

    // --- ライフサイクル関数（必要に応じてオーバーライド） ---
    // void OnEnter() override;   // アクティブになった時
    // void OnExit() override;    // 非アクティブ・破棄される直前
    // void OnSuspend() override; // 上に別のシーンがPushされた時
    // void OnResume() override;  // 上のシーンがPopされ最前面に戻った時

private:
    std::unique_ptr<ObjClass> playerObj_;
};
```

#### シーン遷移と重ね合わせ（Push / Pop）
シーンを完全に移動するには `TransitionTo` を使いますが、ポーズ画面のように**現在のシーンを残したまま一時的な画面を重ねる**場合は `PushScene` を使います。
```cpp
// 完全に別のシーンへ移動する場合
if (isClear) {
    // "ClearScene" に遷移する。トランジション効果は Fade で 1.0秒かける
    engine_->GetSceneManager()->TransitionTo("ClearScene", SceneTransition::Type::Fade, 1.0f);
}

// 現在のシーンの上に一時的なシーン（ポーズ画面など）を重ねる場合
// engine_->GetSceneManager()->PushScene("PauseScene");

// 重ねたシーンを終了し、元のシーンに戻る場合（PauseScene側で呼ぶ）
// engine_->GetSceneManager()->PopScene();
```

### 1.3 RenderGraph と DrawManager (描画パイプライン)
本エンジンの描画は **RenderGraph（レンダーグラフ）** という仕組みで自動管理されています。
ユーザーが `Draw()` を呼ぶと、すぐに画面に描画されるわけではなく、**DrawManagerのキュー（予約リスト）に登録（Submit）** されます。その後、エンジン側が適切な順序（Opaque → Transparent → UIなど）でまとめてGPUへ描画命令を出します。

### 1.4 カメラの操作 (CameraManager / Camera)
3D空間を描画するための「視点」を管理します。

```cpp
auto* cameraManager = engine_->GetCameraManager();
auto* camera = cameraManager->GetActiveCamera();

// カメラの位置と注視点を設定
camera->SetTranslate({ 0.0f, 5.0f, -10.0f });
camera->SetTarget({ 0.0f, 0.0f, 0.0f });

// 行列を更新（位置を変更したら必ず呼ぶ）
camera->UpdateMatrix();

// ※デバッグカメラへの切り替えについて
// 以前は CameraManager で切り替えていましたが、現在は BaseScene に統合されています。
// ImGuiのデバッグタブ「Camera & Lights」から "Debug Camera Mode" のチェックを入れるか、
// コード内で `isDebugCameraMode_ = true;` とすることでデバッグカメラが有効になります。
```

---

## 2. 描画・オブジェクトシステム (Rendering System)

画面にモノを表示するための主要なクラス群です。

### 2.1 3Dモデル描画 (`ObjClass` / `AnimationModel`)
静的な3Dモデルを表示するには `ObjClass` を使います。モデルデータは `ModelManager` を経由して自動的にキャッシュされます。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<ObjClass> model_;

// 2. 初期化 (Initialize)
model_ = std::make_unique<ObjClass>();
model_->Initialize("enemy/enemy.obj"); // resources/model/ 以下のパスを指定
model_->SetPosition({ 0.0f, 0.0f, 10.0f });
model_->SetScale({ 2.0f, 2.0f, 2.0f });

// 3. 更新 (Update)
model_->Update(); // ※毎フレーム必ず呼ぶこと（ワールド行列が更新されます）

// 4. 描画 (Draw)
model_->Draw();   // DrawManagerの標準3D描画キューに登録される
```

### 2.2 2Dスプライト描画 (`Sprite`)
画面にUIなどの2D画像を表示するためのクラスです。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<Sprite> sprite_;

// 2. 初期化 (Initialize)
sprite_ = std::make_unique<Sprite>();
sprite_->Initialize("ui/title_logo.png"); // resources/ 以下のパスを指定
sprite_->SetPosition({ 640.0f, 360.0f }); // 画面中央 (1280x720の場合)
sprite_->SetAnchorPoint({ 0.5f, 0.5f });  // 画像の中心を基準にする

// 3. 更新 (Update)
sprite_->Update();

// 4. 描画 (Draw)
sprite_->Draw(); // 通常のUIパスで描画される

// ※ポストプロセス（ブルーム等）の影響を受けない最前面UIとして描画したい場合
// sprite_->Draw(true); 
```

### 2.3 プリミティブ形状 (`CubeClass`, `SphereClass`, `CylinderClass`, `LineClass`)
当たり判定のデバッグ表示や、プロトタイプの作成に便利な組み込み図形です。モデルファイルなしで使えます。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<CubeClass> cube_;
std::unique_ptr<LineClass> line_;

// 2. 初期化 (Initialize)
cube_ = std::make_unique<CubeClass>();
cube_->Initialize();
cube_->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f }); // 赤色で半透明

line_ = std::make_unique<LineClass>();
line_->Initialize();
line_->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f }); // 緑色の線

// 3. 更新と描画 (Update & Draw)
cube_->SetPosition(playerPos);
cube_->Update();
cube_->Draw();

line_->SetStartAndEnd(startPos, endPos);
line_->Update();
line_->Draw(); // ライン専用のキューに登録される
```

### 2.4 パーティクル (`GPUParticleSystem`)
コンピュートシェーダを利用して数万個のパーティクルを高速に描画するシステムです。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<GPUParticleSystem> gpuParticle_;

// 2. 初期化 (Initialize)
gpuParticle_ = std::make_unique<GPUParticleSystem>();
gpuParticle_->Initialize("effect/particle_tex.png");
gpuParticle_->SetColor({ 1.0f, 0.5f, 0.1f, 1.0f });
gpuParticle_->SetParticleLife(0.5f, 1.0f); // 寿命(最小, 最大)

# IrufemiEngine 取扱説明書 (Manual)

このドキュメントは、IrufemiEngineを利用してゲームを開発するチームメンバーのための総合マニュアルです。
各機能の役割と、すぐに使えるコードスニペット（コピペ用コード）をまとめています。

---

## 0. プロジェクト構造とコーディングルール (Project Structure)

新しくコードを書いたり、リソースを追加する際は以下の配置ルールに必ず従ってください。

- **`IrufemiEngine/` (エンジンコア)**
  - 描画パイプラインや汎用的なマネージャーが置かれます。**ゲーム特有のロジックやアクターは絶対にここに書かないでください。**
- **`Application/` (ゲームロジック)**
  - プレイヤーの動き、敵のAI、各種シーン（Title, InGame等）の処理はすべてここに作成します。
- **`resources/` (リソースデータ)**
  - 3Dモデル（`.obj`, `.gltf`）やテクスチャ（`.png`）、音声（`.wav`）は必ずこのフォルダ以下の適切なディレクトリ（`model/`, `ui/`, `audio/` 等）に配置してください。

---

## 1. エンジンの基本アーキテクチャ (Core Architecture)

### 1.1 IrufemiEngine クラス
エンジン全体を統括するコアクラスです。`WinApp`（ウィンドウ管理）や `DirectXCommon`（DirectX12初期化）を保持し、メインループ（`Update` と `Draw`）を回します。ゲームアプリケーション全体で1つのインスタンスのみが存在します。

### 1.2 SceneManager と IScene (シーン管理)
ゲームの画面（タイトル、インゲーム、リザルトなど）を切り替えるための仕組みです。
新しいシーンを追加する場合は `IScene` または `BaseScene` を継承したクラスを作成します。

#### シーンの作り方
```cpp
#pragma once
#include "Framework/BaseScene.h"
#include <memory>
class ObjClass;

class ExampleScene : public BaseScene {
public:
    ~ExampleScene() override = default;
    
    // シーン遷移時に一度だけ呼ばれる（リソース読み込みなど）
    void Initialize(IrufemiEngine* engine) override;
    
    // 毎フレーム呼ばれる（ロジックの更新）
    void Update() override;
    
    // 毎フレーム呼ばれる（描画リクエストの送信）
    void Draw() override;

    // --- ライフサイクル関数（必要に応じてオーバーライド） ---
    // void OnEnter() override;   // アクティブになった時
    // void OnExit() override;    // 非アクティブ・破棄される直前
    // void OnSuspend() override; // 上に別のシーンがPushされた時
    // void OnResume() override;  // 上のシーンがPopされ最前面に戻った時

private:
    std::unique_ptr<ObjClass> playerObj_;
};
```

#### シーン遷移（自動データ駆動遷移と手動遷移）
シーンを切り替えるには主に2つの方法があります。

**1. データ駆動の自動遷移 (推奨)**
あらかじめエディタで作った `Title.json` などを指定して画面を遷移します。C++で専用のシーンクラスを作る必要はありません。
```cpp
// "Title" (Title.json) を読み込んで遷移。トランジション効果は Fade で 1.0秒かける
engine_->GetSceneManager()->LoadScene("Title", SceneTransition::Type::Fade, 1.0f);
```

**2. C++クラスによる手動遷移**
`BaseScene` を継承した専用クラスへ遷移します。
```cpp
if (isClear) {
    // 登録済みの "ClearScene" クラスに遷移する
    engine_->GetSceneManager()->TransitionTo("ClearScene", SceneTransition::Type::Fade, 1.0f);
}

// 現在のシーンの上に一時的なシーン（ポーズ画面など）を重ねる場合
// engine_->GetSceneManager()->PushScene("PauseScene");

// 重ねたシーンを終了し、元のシーンに戻る場合（PauseScene側で呼ぶ）
// engine_->GetSceneManager()->PopScene();
```

### 1.3 RenderGraph と DrawManager (描画パイプライン)
本エンジンの描画は **RenderGraph（レンダーグラフ）** という仕組みで自動管理されています。
ユーザーが `Draw()` を呼ぶと、すぐに画面に描画されるわけではなく、**DrawManagerのキュー（予約リスト）に登録（Submit）** されます。その後、エンジン側が適切な順序（Opaque → Transparent → UIなど）でまとめてGPUへ描画命令を出します。

### 1.4 カメラの操作 (CameraManager / Camera)
3D空間を描画するための「視点」を管理します。

```cpp
auto* cameraManager = engine_->GetCameraManager();
auto* camera = cameraManager->GetActiveCamera();

// カメラの位置と注視点を設定
camera->SetTranslate({ 0.0f, 5.0f, -10.0f });
camera->SetTarget({ 0.0f, 0.0f, 0.0f });

// 行列を更新（位置を変更したら必ず呼ぶ）
camera->UpdateMatrix();

// ※デバッグカメラへの切り替えについて
// 以前は CameraManager で切り替えていましたが、現在は BaseScene に統合されています。
// ImGuiのデバッグタブ「Camera & Lights」から "Debug Camera Mode" のチェックを入れるか、
// コード内で `isDebugCameraMode_ = true;` とすることでデバッグカメラが有効になります。
```

---

## 2. 描画・オブジェクトシステム (Rendering System)

画面にモノを表示するための主要なクラス群です。

### 2.1 3Dモデル描画 (`ObjClass` / `AnimationModel`)
静的な3Dモデルを表示するには `ObjClass` を使います。モデルデータは `ModelManager` を経由して自動的にキャッシュされます。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<ObjClass> model_;

// 2. 初期化 (Initialize)
model_ = std::make_unique<ObjClass>();
model_->Initialize("enemy/enemy.obj"); // resources/model/ 以下のパスを指定
model_->SetPosition({ 0.0f, 0.0f, 10.0f });
model_->SetScale({ 2.0f, 2.0f, 2.0f });

// 3. 更新 (Update)
model_->Update(); // ※毎フレーム必ず呼ぶこと（ワールド行列が更新されます）

// 4. 描画 (Draw)
model_->Draw();   // DrawManagerの標準3D描画キューに登録される
```

### 2.2 2Dスプライト描画 (`Sprite`)
画面にUIなどの2D画像を表示するためのクラスです。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<Sprite> sprite_;

// 2. 初期化 (Initialize)
sprite_ = std::make_unique<Sprite>();
sprite_->Initialize("ui/title_logo.png"); // resources/ 以下のパスを指定
sprite_->SetPosition({ 640.0f, 360.0f }); // 画面中央 (1280x720の場合)
sprite_->SetAnchorPoint({ 0.5f, 0.5f });  // 画像の中心を基準にする

// 3. 更新 (Update)
sprite_->Update();

// 4. 描画 (Draw)
sprite_->Draw(); // 通常のUIパスで描画される

// ※ポストプロセス（ブルーム等）の影響を受けない最前面UIとして描画したい場合
// sprite_->Draw(true); 
```

### 2.3 プリミティブ形状 (`CubeClass`, `SphereClass`, `CylinderClass`, `LineClass`)
当たり判定のデバッグ表示や、プロトタイプの作成に便利な組み込み図形です。モデルファイルなしで使えます。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<CubeClass> cube_;
std::unique_ptr<LineClass> line_;

// 2. 初期化 (Initialize)
cube_ = std::make_unique<CubeClass>();
cube_->Initialize();
cube_->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f }); // 赤色で半透明

line_ = std::make_unique<LineClass>();
line_->Initialize();
line_->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f }); // 緑色の線

// 3. 更新と描画 (Update & Draw)
cube_->SetPosition(playerPos);
cube_->Update();
cube_->Draw();

line_->SetStartAndEnd(startPos, endPos);
line_->Update();
line_->Draw(); // ライン専用のキューに登録される
```

### 2.4 パーティクル (`GPUParticleSystem`)
コンピュートシェーダを利用して数万個のパーティクルを高速に描画するシステムです。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<GPUParticleSystem> gpuParticle_;

// 2. 初期化 (Initialize)
gpuParticle_ = std::make_unique<GPUParticleSystem>();
gpuParticle_->Initialize("effect/particle_tex.png");
gpuParticle_->SetColor({ 1.0f, 0.5f, 0.1f, 1.0f });
gpuParticle_->SetParticleLife(0.5f, 1.0f); // 寿命(最小, 最大)

// 3. 放出設定と更新 (Update)
// 発生源の位置、進行方向、広がり、速度、拡散、1フレームの発生数
gpuParticle_->SetBeamEmitter(position, direction, 1.0f, 0.5f, 0.1f, 100);
gpuParticle_->SetEmit(true); // 放出ON
gpuParticle_->Update();

// 4. 描画 (Draw)
gpuParticle_->Draw();
```

### 2.5 コンポーネントシステム (Component System)
`ObjClass` などの単体クラスに代わる、モダンなオブジェクト構築手法です。`GameObject` に必要な `Component` を組み合わせて機能を構築します。

```cpp
// 1. GameObjectの生成
auto object = std::make_shared<GameObject>("MyEntity");

// 2. コンポーネントのアタッチ
auto* transform = object->AddComponent<TransformComponent>();
auto* renderer = object->AddComponent<MeshRendererComponent>();

// 3. パラメータ設定
transform->SetPosition({0, 10, 0});
renderer->LoadModel("enemy/boss.obj");

// 4. シーンへの登録（SceneManager経由で管理する場合）
// scene->AddGameObject(object);
```

#### 新規コンポーネント（スクリプト）作成時のルールと自動UI化
エンジンの「簡易リフレクション」機能により、変数を宣言して `OnRegisterProperties` に書くだけで、**自動でインスペクターにUIが作成され、JSONに保存・復元される**ようになります。手動で ImGui を書いたりシリアライズ関数をオーバーライドする必要はありません。

```cpp
#pragma once
#include "Framework/Component/Component.h"

class PlayerStatusComponent : public Component {
public:
    int hp_ = 100;
    float speed_ = 5.0f;

    // 1. コンポーネント名を返す
    std::string GetComponentName() const override { return "PlayerStatusComponent"; }

    // 2. 変数をリフレクションシステムに登録する
    void OnRegisterProperties() override {
        RegisterProperty("Max HP", &hp_);
        RegisterProperty("Move Speed", &speed_);
    }
};
```
*※作成したスクリプトは `ComponentFactory.cpp` の `RegisterAllCoreComponents` で登録するか、同等の場所でファクトリに登録してください。*

#### 衝突判定とコールバック (OnCollisionEnter / Destroy)
ゲームロジックとして「何かにぶつかったら壊れる」「ダメージを受ける」といった処理は、Component 内の仮想関数をオーバーライドして実装します。

```cpp
#include "Framework/Component/Component.h"
#include "Framework/GameObject.h"

class BulletComponent : public Component {
public:
    std::string GetComponentName() const override { return "BulletComponent"; }

    // 当たり判定が行われ、他のコライダーと接触した瞬間に自動で呼ばれます
    void OnCollisionEnter(GameObject* hitObject) override {
        // 相手の名前やComponentを見て処理を分ける
        if (hitObject->GetName() == "Enemy") {
            // Destroy() を呼ぶと、現在のフレームの終わりに安全にオブジェクトが破棄(GC)されます
            GetGameObject()->Destroy();
        }
    }
};
```

#### 動的生成 (Instantiate)
弾を撃つ、敵を出現させるといった「ゲームプレイ中にオブジェクトを生み出す」処理は、あらかじめ作成したプレハブ（JSON）を指定して呼び出します。

```cpp
#include "Framework/Component/Component.h"
#include "Framework/GameObject.h"

class PlayerShooterComponent : public Component {
public:
    std::string GetComponentName() const override { return "PlayerShooterComponent"; }

    void Update() override {
        // 例: 左クリックされたら弾を生成する
        if (engine_->GetInputManager()->IsMouseButtonPressed(Mouse::Button::Left)) {
            // 現在の座標を取得
            Vector3 pos = GetGameObject()->GetComponent<TransformComponent>()->position_;
            
            // "resources/scenes/Bullet.json" をもとに、指定した座標へオブジェクトを生成
            auto bullet = GetGameObject()->Instantiate("Bullet", pos);
        }
    }
};
```

---

## 3. リソース管理 (Resource Management)

ゲームに必要なテクスチャ、3Dモデル、サウンドデータは、エンジン内の各 Manager を通して一元管理（ロード・キャッシュ）されます。

### 3.1 TextureManager (テクスチャ管理)
`Sprite` などの描画に必要な画像をロードし、GPU用のハンドルを取得します。同じ画像を何度ロードしても、1度だけメモリに乗るようになっています。

```cpp
// 1. 画像のロード (resources/ フォルダからの相対パス)
engine_->GetTextureManager()->Load("ui/title_logo.png");

// 2. フォルダ内の画像一括ロード (ローディング画面などで使用)
engine_->GetTextureManager()->LoadAllFromFolder("resources/ui");

// ※ Sprite等の Initialize にファイル名を渡せば、内部で自動的にロードされます。
```

### 3.2 ModelManager (3Dモデル管理)
`.obj` や `.gltf` 形式の3Dモデルを読み込み、最適化してキャッシュします。

```cpp
// モデルの事前ロード（インゲーム開始前などに呼ぶとカクつきを防げます）
engine_->GetModelManager()->LoadModel("enemy/enemy.obj");

// ※ ObjClass::Initialize("enemy/enemy.obj") を呼べば内部で自動ロードされます。
```

### 3.3 AudioManager (サウンド管理)
BGMやSEの再生・停止・音量調節を行います。

```cpp
// 1. サウンドの事前ロード
engine_->GetAudioManager()->GetOrLoadSoundByFile("resources/audio/bgm_title.wav");

// 2. フォルダごとの一括ロード (カテゴリ分け)
engine_->GetAudioManager()->LoadSoundsFromFolder("resources/audio/se", "SE");

// 3. 再生
auto soundData = engine_->GetAudioManager()->GetOrLoadSoundByFile("resources/audio/bgm_title.wav");
// (サウンドデータ, ループフラグ, 音量 0.0f~1.0f)
std::weak_ptr<VoiceInstance> voice = engine_->GetAudioManager()->Play(soundData, true, 0.8f);

// 4. 停止
engine_->GetAudioManager()->Stop(voice);
```

---

## 4. 入力・演算・ユーティリティ (Input & Utility)

プレイヤーの操作を受け取ったり、ゲームに必要な計算を行うクラス群です。

### 4.1 InputManager と BaseScene の入力ヘルパー (入力の取得)
キーボード、マウス、ゲームパッド（XInput互換）の操作を取得できます。
`BaseScene` を継承したクラスでは、直接 `PressedVK()` や `IsButtonPressed()` などのヘルパー関数を呼び出すのが最も簡単です。

```cpp
// 【キーボード (BaseScene内での使用例)】
// Spaceキーが「押された瞬間」か？ (VK: 仮想キーコード)
if (PressedVK(VK_SPACE)) { ... }
// Wキーが「押され続けている」か？ (DIK: DirectInput互換キーコード)
if (DownDIK(DIK_W)) { ... }

// 【ゲームパッド (BaseScene内での使用例)】
// Aボタンが押された瞬間か？
if (IsButtonPressed(XINPUT_GAMEPAD_A)) { ... }

// 左スティックの入力値 (-1.0f 〜 1.0f) ※入力値取得は InputManager 経由で行います
auto input = engine_->GetInputManager();
float lx = input->GetLeftStickX();
float ly = input->GetLeftStickY();

// 【マウス (InputManager経由)】
// 左クリックされたか？
if (input->IsMouseButtonPressed(Mouse::Button::Left)) { ... }
// マウスの現在座標 (スクリーン座標)
Vector2 mousePos = input->GetMousePosition();
```

### 4.2 Math ユーティリティ (算術演算)
`Math` 名前空間に関数がまとまっています。`Vector3` や `Matrix4x4` を安全に計算します。

```cpp
#include "Core/Math/Math.h"

Vector3 playerPos = { 0.0f, 0.0f, 0.0f };
Vector3 enemyPos  = { 10.0f, 0.0f, 10.0f };

// 距離の計算
Vector3 diff = Math::Subtract(enemyPos, playerPos);
float distance = Math::Length(diff);

// 正規化 (方向ベクトルの取得)
Vector3 direction = Math::Normalize(diff);

// 加算と乗算
Vector3 offset = Math::Multiply(5.0f, direction);
Vector3 targetPos = Math::Add(playerPos, offset);
```

### 4.3 Collision (当たり判定)
`OBB` (有向境界箱) や `Sphere` などの定義と交差判定を行います。

```cpp
#include "Core/Math/Geometry/OBB.h"
#include "Core/Math/Geometry/Sphere.h"

// 衝突判定の実装例
if (IsCollision(playerOBB, enemySphere)) {
    // プレイヤーと敵の球が当たった時の処理
}
```

### 4.4 UIメニュー構築の定石 (UISelectionGroup)
タイトル画面やポーズメニューなど、「上下で選んで決定する」UIを作るための便利なクラスです。手動でカーソル管理や長押し防止を書く必要がなくなります。

```cpp
#include "Framework/Overlay/UISelectionGroup.h"

// 1. メンバ変数に定義
UISelectionGroup uiGroup_;

// 2. Initialize() で設定
// (要素数, 初期選択インデックス, 横並びか, 連続入力の間隔, ループするか)
uiGroup_.Initialize(3, 0, false, 0.2f, true); 

// 3. Update() で入力判定
uiGroup_.Update(engine_->GetInputManager());

if (uiGroup_.IsTriggered()) {
    // 現在選ばれている項目を取得
    int selected = uiGroup_.GetSelectedIndex();
    if (selected == 0) {
        // 「ゲーム開始」が選ばれた
    } else if (selected == 1) {
        // 「設定」が選ばれた
    }
}

// ※描画時は uiGroup_.GetSelectedIndex() に応じて、選ばれている項目の色やスプライトを変えます。
```

---

## 5. 高度な機能と拡張 (Advanced Features)

ゲームをさらにリッチにするためのポストプロセス設定や、デバッグ用のUI機能、およびエンジンを独自に拡張する際のお作法です。

### 5.1 ポストプロセス (PostProcessManager)
画面全体にさまざまなエフェクト（ブルーム、ビネット、ノイズ、ディゾルブなど）をかけます。複数のエフェクトをスタック（重ね掛け）することが可能です。

```cpp
auto* pp = engine_->GetPostProcessManager();

// 1. エフェクトのリセットと適用
pp->ClearActiveModes();
pp->AddActiveMode(PostProcessMode::Bloom);
pp->AddActiveMode(PostProcessMode::Vignette);

// 2. パラメータの調整
// ブルーム（発光）の強度を調整
pp->GetBloomParams().intensity = 1.2f;
// ビネット（画面端の暗転）の強さを調整
pp->GetVignetteParams().power = 0.8f;

// ※ シーン遷移時のフェードなどもこれを利用して実装できます
```

### 5.2 デバッグ機能 (DebugUI / ImGui)
パラメータの調整や変数の確認を行うために、ImGuiを利用してデバッグウィンドウを表示できます。
`BaseScene` を継承したクラスでは、`DrawDebugTab()` をオーバーライドするだけで自動的にタブが追加されます。

```cpp
void ExampleScene::DrawDebugTab() {
#if defined USE_IMGUI
    // 親クラスの処理を呼ぶ
    BaseScene::DrawDebugTab();

    // 独自のタブを追加
    if (ImGui::BeginTabItem("Example Scene")) {
        // パラメータ調整用スライダー
        static float playerSpeed = 5.0f;
        ImGui::SliderFloat("Player Speed", &playerSpeed, 1.0f, 10.0f);

        // ボタンの配置
        if (ImGui::Button("Reset Player")) {
            playerObj_->SetPosition({0, 0, 0});
        }
        ImGui::EndTabItem();
    }
#endif
}
```

### 5.3 カスタム機能の追加ルール (エンジンの拡張)
エンジンに新しい機能を追加する際は、以下のルールを守ってください。

1. **アーキテクチャの分離**
   - エンジンコア (`IrufemiEngine/`) には、特定のゲームロジック（プレイヤークラスや固有のシーン）を含めないでください。
   - ゲームロジックは必ず `Application/` ディレクトリ配下に作成します。

2. **新しい描画表現 (RenderPass) の追加**
   - 独自のシェーダーや特殊な描画パイプラインを追加したい場合は、`DrawManager` に直接書くのではなく、`IRenderPass` を継承したクラスを作成し、`RenderGraph` に登録してください。
   - これにより、他の描画パスに影響を与えずに安全に機能を追加できます。

3. **ドキュメントの更新**
   - `Manager` の関数名を変えたり、全く新しい機能（例: ECSシステム）を導入した場合は、必ずこの `Manual.md` も合わせて更新し、チームメンバーがすぐに使えるようにしてください。

---

## 6. エディタ (IrufemiEngine Editor) の使い方

### Project Browser の新機能
IrufemiEngine のエディタは、モダンなゲームエンジン（Unity等）ライクなアセット管理をサポートしています。

1. **2ペイン・タイル表示**
   - **左ペイン (ツリー表示)**: フォルダの階層構造のみを表示します。クリックするとその中身が右ペインに表示されます。
   - **右ペイン (タイル表示)**: FontAwesome アイコン付きで、ファイルとフォルダがグリッド状に並びます。画面幅に合わせて自動で折り返されます。

2. **コンテキストメニュー (右クリック操作)**
   - ファイル/フォルダ上で右クリックすると、「**Rename (名前変更)**」や「**Delete (削除)**」が可能です。
   - 余白を右クリックすると、「**Create Folder (新規フォルダ作成)**」ができます。
   - ※これらの操作はOSの実ファイルシステム（Windowsのフォルダ等）に即座に反映されます。

3. **ドラッグ＆ドロップによるアセットの即時配置**
   - **画像の配置**: Project Browser にある `.png` や `.jpg` などの画像を **Hierarchy** にドラッグ＆ドロップすると、`SpriteRendererComponent` がアタッチされた GameObject が自動生成されます。
   - **3Dモデルの配置**: `.obj` や `.fbx` などをドラッグ＆ドロップすると、`MeshRendererComponent` がアタッチされた GameObject が自動生成されます。
   - D&D 時にアセットの相対パスが自動的にコンポーネントへセットされるため、すぐに画面上でプレビュー可能です。

### 6.2 プレハブ (Prefab) システム
Unityライクな「オブジェクトのテンプレート化」をサポートしています。

1. **Prefabの作成・保存**:
   - Hierarchy パネルでオブジェクトを右クリックし、「**Save as Prefab**」を選択します。
   - `resources/prefabs/` 以下に `.prefab.json` として、コンポーネントやパラメータがすべて保存されます。
2. **エディタからの配置**:
   - Project Browser に青いキューブのアイコンで表示されます。これを **Scene View** にドラッグ＆ドロップすると、即座にインスタンス化（実体化）して配置されます。
3. **C++コードからの動的生成 (ランタイム)**:
   - ゲーム中に「敵」や「弾」をスポーンさせるには、C++コードで以下のように呼び出します。
   ```cpp
   #include "Framework/SceneSerializer.h"

   // Prefabをロードしてインスタンス化
   auto bullet = SceneSerializer::LoadPrefab("resources/prefabs/Bullet.prefab.json");
   if (bullet) {
       bullet->GetComponent<TransformComponent>()->position_ = spawnPos;
       scene->AddGameObject(bullet);
   }
   ```

---
> **ドキュメント更新履歴**
> - 2026/05: RenderGraph / パケット分離対応を反映、Phase 1~3 および 便利機能（カメラ、ディレクトリ構造、UISelectionGroup）を追記
> - 2026/05: エディタ機能（Project Browserの2ペイン化、FontAwesome対応、ドラッグ＆ドロップ機能）の解説を追記
> - 2026/05: コンポーネントシステム（GameObject/Component）の導入と、UIロジックの分離（Registry方式）を追記
