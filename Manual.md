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

// ※特殊なエフェクト（カスタムPSO）をモデル表面に適用する場合の例
// model_->GetResource()->SetCustomPSO(engine_->GetPSOManager()->GetPSO("EnergyCore", BlendMode::kBlendModeNormal, DepthWrite::Enable, CullMode::Back));
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

#### 特定の GameObject を名前で探す方法
シーン内に存在する特定の GameObject（Player や 各種 Manager など）を取得したい場合、`GetGameObjects()` でループを回す必要はありません。
`BaseScene` に用意されている `FindGameObject` を使用して1行で取得できます。

**コード例:**
```cpp
// 自身の所属するシーンを取得
auto scene = gameObject_->GetScene();
if (scene) {
    // "Player" という名前のオブジェクトを検索
    auto playerObj = scene->FindGameObject("Player");
    if (playerObj) {
        auto transform = playerObj->GetComponent<TransformComponent>();
        // ...
    }
}
```
※注意：同名のオブジェクトが複数存在する場合は、最初に見つかったものが返されます。

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

### 2.3 プリミティブ形状 (`Primitive3DObject`, `LineClass`)
当たり判定のデバッグ表示や、プロトタイプの作成に便利な組み込み図形です。
以前は `CubeClass` や `SphereClass` などの専用クラスに分かれていましたが、現在は `Primitive3DObject` に統合されており、１つのクラスで複数の形状（Cube, Sphere, Cylinder, Plane, Torus 等）を自由に切り替えて表示できます。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<Primitive3DObject> primitive_;
std::unique_ptr<LineClass> line_;

// 2. 初期化 (Initialize)
primitive_ = std::make_unique<Primitive3DObject>();
// (PrimitiveType::Cube などを指定して初期化)
primitive_->Initialize(PrimitiveType::Cube);
// 形状固有のパラメータ変更 (例：Cubeの場合は不要だがSphereならRadiusを変える等)
primitive_->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f }); // 赤色で半透明

line_ = std::make_unique<LineClass>();
line_->Initialize();
line_->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f }); // 緑色の線

// 3. 更新と描画 (Update & Draw)
// TransformComponentのように直接 transform 情報を更新する（簡易版）
primitive_->transform_.translate = playerPos;
primitive_->Update();
primitive_->Draw();

line_->SetStartAndEnd(startPos, endPos);
line_->Update();
line_->Draw(); // ライン専用のキューに登録される
```

※ **内部データ構造の変更について**：
これまで使われていた `MeshModule` や `MaterialModule` は、それぞれ `MeshDesc` と `MaterialDesc` に名称変更され、`Renderer/Data/RenderData.h` に統合されています。描画パイプラインのコードを独自にカスタマイズする際はこの変更にご注意ください。

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

#### 新規コンポーネント作成時のルール（自動UI化とメニュー登録）
エンジンの「簡易リフレクション」と「動的メニュー生成」により、コンポーネントを追加する際の手間が大幅に削減されています。
変数を宣言して `OnRegisterProperties` に書くだけで、**自動でインスペクターにUIが作成され、JSONに保存・復元される**ようになります。手動で ImGui を書く必要はありません。
さらに、作成したクラスを `ComponentFactory` に登録するだけで、エディタの「Add Component」メニューにも自動で追加されます（エディタのソースコードを変更する必要はありません）。

```cpp
#pragma once
#include "Framework/Component/Component.h"

class PlayerStatusComponent : public Component {
public:
    int hp_ = 100;
    float speed_ = 5.0f;

    // 1. コンポーネント名を返す
    // ※この名前に特定のキーワードを含めることで、Add Component メニューのカテゴリが自動で決まります。
    // （例："Renderer" や "Emitter" -> Renderer枠, "Collider" -> Collider枠, "UI", "Button" -> UI枠）
    std::string GetComponentName() const override { return "PlayerStatusComponent"; }

    // 2. 変数をリフレクションシステムに登録する
    void OnRegisterProperties() override {
        RegisterProperty("Max HP", &hp_);
        RegisterProperty("Move Speed", &speed_);
    }
};
```
*※作成したコンポーネントは、必ず `ComponentFactory.cpp` の `RegisterAllCoreComponents` 関数内（またはゲーム側の初期化処理）でファクトリに登録してください。*
```cpp
// 登録例
ComponentFactory::Register("PlayerStatusComponent", []() { return std::make_shared<PlayerStatusComponent>(); });
```

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

#### 演出コンポーネント (AudioSource / ParticleEmitter)
BGMやSEを鳴らしたり、エフェクトを発生させるには、インスペクターからプロパティを設定するだけで動く以下のコンポーネントが便利です。C++でコードを書く必要すらありません。

- **`AudioSourceComponent`**: アタッチしたオブジェクトから音を鳴らします。
  - `Audio Path`: 再生したい音声ファイルのパス（例: `audio/se/boom.wav`）。
  - `Play On Awake`: シーン開始時に自動で再生するか。
  - `Loop`: ループ再生するか。
- **`ParticleEmitterComponent`**: アタッチしたオブジェクトの位置でエフェクトを発生させます。
  - `Texture Path`: パーティクルに使う画像パス。
  - `Emit Type`: 0(球体), 1(ビーム), 2(ボックス), 3(円柱) などの放出形状。
  - `Color`, `Velocity`, `Emit Count` などで自由にエフェクトを構築できます。

#### UIコンポーネント (Canvas / Button / Text)
ゲーム内の2D UIを構築するための専用コンポーネントです。

- **`TextRendererComponent`**: 高品質なMSDF（マルチチャンネル符号付き距離場）フォントを利用し、任意の文字列を描画するコンポーネントです。動的に必要な文字だけを生成・パッキングするため、メモリ効率が非常に高く、どれだけ拡大しても文字がジャギりません（ぼやけません）。
  - `Text`: 表示する文字列。複数行の入力（改行）にも対応しています。
  - `Font ID`: 使用するフォント名（`resources/fonts/` 以下に配置したファイル名、例：`toro_glitch`）。
  - `Base Scale`: 文字の大きさ。
  - `Color`: 文字の色と透明度。
  - `Top Most`: ブルームなどのポストプロセスの影響を受けない、最前面レイヤーに描画するかどうか。
  - `Alignment`: テキストのアライメント（`0=Left`, `1=Center`, `2=Right`）。複数行の場合は行ごとに適用されます。

- **`ButtonComponent`**: マウスカーソルのホバーやクリックを検知し、色を変えたりシーンを遷移させる機能を提供します。
  - `Load Scene Name`: クリック時に遷移させたいシーン名（例: `InGame`）。空欄の場合は何もしません。
  - `Transition Type(0-3)`: `0=Fade`, `1=Dissolve`, `2=Slide`, `3=RadialBlur` のいずれかを指定し、遷移時の演出（ポストプロセス）を選択できます。
  - `Transition Duration`: 遷移演出にかける時間（秒）。
  - `Normal/Hover/Click Color`: マウスの操作状態に合わせて、アタッチされているSpriteの色を自動的に変化させます。
- **`CanvasComponent`**: UI要素をグループ化し、アルファ値（透明度）などを一括管理します。
  - `Group Alpha`: このコンポーネントを持つGameObject自身と、そのすべての子要素にある `SpriteRendererComponent` のアルファ値を一括で制御します。フェードイン・フェードアウトの演出に便利です。
### 2.5 汎用エフェクトシステム (`Effect`) と 3D爆発エフェクト (`kExplosion`)

敵や障害物に弾丸・ミサイルが着弾した際に使用する、リッチな3D爆発エフェクト機能です。3D球体の急速膨張による炎コア、3軸クロス展開される衝撃波リング、全方位に飛び散るGPU火花パーティクルが統合されています。

#### プレイヤーでの事前生成とプール管理の例

弾丸が連射されたり、同時に多数ヒットした場合に備え、事前にエフェクトオブジェクトをプールしておき使い回す設計が推奨されます。

```cpp
// --- ヘッダー (Player.h) ---
#include "Renderer/Effect/Effect.h"
#include <vector>
#include <memory>

class Player {
private:
    std::vector<std::unique_ptr<Effect>> explosionEffects_;
    static const int kMaxExplosionEffects = 32;
public:
    void Initialize(InputManager* input, IrufemiEngine* engine);
    void Update();
    void Draw();
    void PlayExplosion(const Vector3& position);
};

// --- 実装 (Player.cpp) ---
void Player::Initialize(InputManager* input, IrufemiEngine* engine) {
    // プールを事前に生成・初期化
    explosionEffects_.clear();
    for (int i = 0; i < kMaxExplosionEffects; ++i) {
        auto effect = std::make_unique<Effect>();
        effect->Initialize(EffectType::kExplosion);
        explosionEffects_.push_back(std::move(effect));
    }
}

void Player::Update() {
    // アクティブなエフェクトのみ毎フレーム更新
    for (auto& effect : explosionEffects_) {
        if (effect->IsActive()) {
            effect->Update();
        }
    }
}

void Player::Draw() {
    // アクティブなエフェクトの描画・同期
    for (auto& effect : explosionEffects_) {
        if (effect->IsActive()) {
            effect->SyncBeforeDraw();
            effect->Draw();
        }
    }
}

void Player::PlayExplosion(const Vector3& position) {
    // プールから空いているエフェクトを探して再生開始
    for (auto& effect : explosionEffects_) {
        if (!effect->IsActive()) {
            effect->Play(position);
            break;
        }
    }
}
```

#### 着弾時の呼び出し例 (GameScene.cpp など)

```cpp
if (Collision::IsOBBSphereCollision(part->GetOBB(), bulletSphere)) {
    bullets[i].isActive = false;
    // 着弾位置に爆発エフェクトを発生
    player_->PlayExplosion(bullets[i].position);
}
```

### 2.6 カスタムパラメータの渡し方 (Custom Constant Buffer)
エンジン標準の `Material` には含まれない独自のパラメータ（演出用の色やアニメーションフラグなど）をシェーダーに渡したい場合、エンジンの `Material` を汚染するのではなく、専用の定数バッファ枠 (`RootSlot::Special` / レジスタ `b6`) を利用します。

```cpp
// 1. 専用の構造体と定数バッファを定義 (16バイトアライメントを意識)
struct MyCustomParams {
    Vector4 customColor;
    float param1;
    float padding[3];
};

std::unique_ptr<DynamicConstantBuffer<MyCustomParams>> myCb_;
MyCustomParams params_{};

// 2. 初期化時にバッファを生成し、描画オブジェクトにGPUアドレスをセット
myCb_ = std::make_unique<DynamicConstantBuffer<MyCustomParams>>();
myCb_->Initialize(engine_->GetDXCommon(), 1);
myCb_->Update(params_);

model_->GetResource()->SetCustomCBVAddress(myCb_->GetGPUVirtualAddress());

// 3. HLSL (シェーダー) 側で受け取る
// ConstantBuffer<MyCustomParams> gMyParams : register(b6);
```

### 2.7 マルチバッファ同期と基底クラス (`MultiBufferSyncState`)
DirectX 12 でフレーム間のマルチバッファリング（`kMaxFramesInFlight`）を行う際、CPUからGPUへの定数バッファの更新タイミングを管理するために `MultiBufferSyncState` 基底クラスを利用します。
`BaseResource` や `BaseModel` などの描画リソースクラスは、すでにこのクラスを継承しています。

#### 使い方ルール
1. **CPU側でのデータ変更時 (`Update` 等)**
   描画オブジェクトのパラメータ（座標、色、カスタムマテリアル等）を変更した際は、必ず `MarkAsDirty()` を呼び出します。これにより、全フレームバッファ（最大3フレーム分）に「更新が必要」というフラグが立ちます。
   ```cpp
   void Skybox::Update() {
       // 行列や色の計算...
       transformationMatrix_.World = worldMatrix;
       
       // データが変更されたので全フレームバッファのDirtyフラグを立てる
       MarkAsDirty();
   }
   ```
2. **GPUバッファへの同期時 (`SyncBeforeDraw` 等)**
   GPUにデータを書き込む直前（描画の直前）で `CheckAndClearDirty(frameIndex)` を呼び出します。この関数は、対象フレームの更新フラグが立っている場合のみ `true` を返し、同時にフラグをクリアします。
   ```cpp
   void Skybox::SyncBeforeDraw() {
       uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
       
       // 対象フレームのバッファが古い場合のみ、GPUへ転送する
       if (CheckAndClearDirty(frameIndex)) {
           transformationBuffer_.Update(transformationMatrix_, frameIndex);
       }
   }
   ```
   ※従来の手動配列管理（`isDirtyBuffer_[frameIndex] = true` 等）は非推奨です。

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

### 3.4 FontManager (フォントとテキストの管理)
TTF等のフォントファイルを読み込み、テキスト描画用の高品質なMSDF画像を動的に生成・キャッシュします。

```cpp
// 1. フォルダ内の全フォント一括ロード
engine_->GetFontManager()->LoadAllFromFolder("resources/fonts");

// 2. 【超重要】スパイク（処理落ち）防止のための事前生成
// ゲーム中に「まだ生成されていない全く新しい文字（複雑な漢字など）」が出現すると、
// メインスレッドでMSDF画像生成が行われ、一瞬ゲームがカクつく（スパイクする）可能性があります。
// プレイスルーを滑らかにするため、シーン開始時のロード中などに「使う予定の全文字」を事前生成してください。
engine_->GetFontManager()->PrecacheText("my_font", L"このシーンで使う予定の会話テキストや、よく使う漢字一覧など...");
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

### 6.3 カスタムエディタUI構築と Undo/Redo (Ctrl+Z) 対応
エンジン標準の `GetProperties()` （簡易リフレクション）を使わずに、複雑なカスタムUIを構築したい場合は、`IComponentEditor` を継承したクラスを作成します（実装は `ComponentEditorRegistry.cpp` に記述）。
この際、インスペクターでの値の変更を Undo/Redo (Ctrl+Z) に対応させるため、以下の専用のヘルパー関数を使用してください。

**1. ドラッグ操作など連続する値の変更 (`CheckUndoRedoDrag`)**
`ImGui::DragFloat` や `ImGui::ColorEdit` など、ドラッグ中に値が連続的に変わるUIでは、操作の「開始」と「終了」のタイミングを自動判定してコマンドを発行する `CheckUndoRedoDrag` を呼び出します。

```cpp
// UI描画と値の更新
if (ImGui::DragFloat("Base Scale", &scale, 0.1f, 0.1f, 1000.0f)) {
    comp->SetBaseScale(scale); // ドラッグ中のリアルタイム反映
}
// ドラッグ終了時に自動でUndo/Redoコマンドを発行
CheckUndoRedoDrag(actionManager, &scale, [comp](const float& v) { 
    comp->SetBaseScale(v); 
});
```

**2. チェックボックスやコンボボックスなど即時確定する変更 (`PushInstantUndo`)**
`ImGui::Checkbox` や `ImGui::BeginCombo` など、クリックした瞬間に値が確定するUIでは、変更前後の値を使って `PushInstantUndo` を呼び出します。

```cpp
bool isTopMost = comp->IsTopMost();
if (ImGui::Checkbox("TopMost", &isTopMost)) {
    // 変更前の値、変更後の値、値をもとに戻すためのセッター関数（ラムダ）を渡す
    PushInstantUndo(actionManager, comp->IsTopMost(), isTopMost, [comp](const bool& v) { 
        comp->SetTopMost(v); 
    });
}
```

---
## 7. トラブルシューティング (Troubleshooting)

### 7.1 アプリケーション終了時に `LIVE_DEVICE` エラーでクラッシュする
**現象**: Visual Studio の出力ウィンドウに `D3D12 WARNING: Live ID3D12Device` と表示され、`D3DResourceLeakChecker` でブレークポイントが止まる。

**原因**: 
- `PrimitiveManager` などのシングルトンクラスが GPU リソース（頂点バッファ等）を保持したまま、DirectX のデバイスが破棄されようとした際に発生します。
- または、`RenderTexture` や `DescriptorPool` から確保したリソースが解放されていない場合や、`TransientResourceManager` 等でフレーム終了時の遅延破棄(`ReleaseAfterFence`) キューに積まれたリソースがアプリケーション終了時にクリアされていない場合に発生します。
- 注意点として、エラー追跡用に追加した `ID3D12InfoQueue` などの COMポインタ自体のスコープが長すぎて、`ReportLiveObjects` の時点でデバイスの参照を保持してしまう（見かけ上のリーク）こともあります。

**解決策**:
- `IrufemiEngine::Finalize()` 内で、`dxCommon_` が破棄される前に、すべてのマネージャーの `Finalize()` または `reset()` を呼ぶようにしてください。シングルトンの場合は `PrimitiveManager::Finalize()` のように明示的に呼び出します。
- **フレーム遅延破棄の注意**: `dxCommon_->ReleaseAfterFence(resource)` で破棄を予約したリソースは、`DirectXCommon::pendingResources_` に保持されます。エンジン終了時には、必ずGPU同期待ち（`WaitForGPU()`）の直後に `pendingResources_.clear()` を呼び出して完全に破棄してください。
- COMポインタ（`ComPtr`）を使用する場合は、不要になったら `Reset()` を呼ぶか、寿命を強制的に限定するためローカルスコープ `{}` 内で宣言するようにしてください。
---

## 8. シェーダーの追加・変更とコンパイル構成 (Shaders & Compilation)

本エンジンでは、パフォーマンスと開発効率の両立のため、ビルド構成（Configuration）によってシェーダーのコンパイル方式が完全に切り替わる「ハイブリッド構成」を採用しています。

### 8.1 開発時のホットリロード (Editor / Debug ビルド)
Editor または Debug モードで起動している場合、ゲームを実行したまま（エディタを立ち上げたまま）、シェーダーファイル（`.hlsl` または `.hlsli`）を上書き保存するだけで、自動的に **ホットリロード** が行われます。
- バックグラウンドでフォルダを監視しており、保存を検知すると安全に古いシェーダーとPSOキャッシュを破棄し、再コンパイルして即座に描画に反映します。
- 一々ビルドし直す必要がないため、ライティングの微調整やエフェクト作成のイテレーションが非常に高速です。

### 8.2 シェーダーの追加手順と命名規則
新しいシェーダー（`.hlsl`）を追加する際は、必ずファイル名の中に「プロファイル名」を含めるようにしてください。ビルド時の自動コンパイルバッチ（`CompileShaders.bat`）がこのファイル名を見て適切なコンパイルを行います。

- **頂点シェーダー**: `xxx.VS.hlsl`
- **ピクセルシェーダー**: `xxx.PS.hlsl`
- **コンピュートシェーダー**: `xxx.CS.hlsl`
- **ジオメトリシェーダー**: `xxx.GS.hlsl`

### 8.3 配布環境 (Release ビルド) と CSOファイル
Releaseビルドでは、`dxcompiler.dll` などのコンパイラを一切ロードせず、最速で起動させる仕組みになっています。
- Visual Studio で F5（またはビルド）を押した直後に、裏で自動的に `CompileShaders.bat` が走り、すべての `.hlsl` をコンパイルして `.cso`（コンパイル済みバイナリ）を生成します。
- 実行時には `.hlsl` ではなく生成された `.cso` を直接メモリに読み込みます。
- そのため、最終的にプレイヤーに配布（リリース）する際は、**「すべての `.hlsl` / `.hlsli` ファイルは削除して `.cso` だけを含める」** ことで、ソースコードの秘匿化と容量削減が可能です（Compute Shaderも含む）。

---
> **ドキュメント更新履歴**
> - 2026/05: RenderGraph / パケット分離対応を反映、Phase 1~3 および 便利機能（カメラ、ディレクトリ構造、UISelectionGroup）を追記
> - 2026/05: エディタ機能（Project Browserの2ペイン化、FontAwesome対応、ドラッグ＆ドロップ機能）の解説を追記
> - 2026/05: コンポーネントシステム（GameObject/Component）の導入と、UIロジックの分離（Registry方式）を追記
> - 2026/05: 高品質テキスト描画システム (`TextRendererComponent` / `FontManager`) と、Play時の自動保存機能を追記
> - 2026/05: 終了時のリソースリーク（LIVE_DEVICE）対策とトラブルシューティングを追記
> - 2026/05: 3D爆発エフェクト (Effect::kExplosion) 仕様とスニペットの追加
> - 2026/06: カスタムエディタUI構築時の Undo/Redo (Ctrl+Z) 対応用ヘルパー関数の解説を追記
> - 2026/06: プリミティブ描画の `Primitive3DObject` への統合、および `MeshDesc` / `MaterialDesc` へのデータ構造リファクタリングを反映
> - 2026/06: 描画リソースのマルチバッファ同期処理を `MultiBufferSyncState` 基底クラスへ集約し、`Manual.md` に使い方を追加
> - 2026/06: シェーダーのハイブリッドコンパイル（Releaseの完全CSO化）およびEditor/Debug向けのホットリロード機能の解説を追加
