# Irufemi Engine

DirectX12をベースにした自作3Dゲームエンジンです。

[![DebugBuild](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DebugBuild.yml)
[![ReleaseBuild](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/ReleaseBuild.yml)
[![DevelopmentBuild](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml/badge.svg)](https://github.com/Irufemi/CG3/actions/workflows/DevelopmentBuild.yml)
[![CheckUnwantedFiles](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml/badge.svg)](https://github.com/Irufemi/Irufemi/actions/workflows/CheckUnwantedFiles.yml)

## 概要

このプロジェクトは、3Dグラフィックスプログラミングの学習を目的として開発されているゲームエンジンです。
DirectX 12のAPIを直接的に利用し、レンダリングの基礎から応用までを実装しています。

## 主な機能

- **レンダリング**
  - DirectX 12ベースのレンダリングパイプライン
  - スプライト、三角形、球、立方体などの基本図形の描画
  - モデルデータ（`.obj`, `.gltf`）の読み込みと描画
  - 平行光源、点光源、スポットライト
  - パーティクルシステム
- **モデルとアニメーション**
  - Assimpライブラリによる3Dモデルのインポート
  - スケルタルアニメーションの再生
- **その他**
  - ImGuiによるデバッグUI
  - 自作の算術ライブラリ（Vector, Matrix, Quaternion）

## 動作環境

- Windows 10 / 11
- Visual Studio 2022
- Windows SDK (バージョン: 10.0.26100.7175 以降)

## ビルド方法

1. このリポジトリをクローンします。
2. `Irufemi.sln` を Visual Studio 2022 で開きます。
3. ソリューションをビルドします。

## 使用ライブラリ

このプロジェクトは、以下のサードパーティ製ライブラリに依存しています。

- [Assimp](https://github.com/assimp/assimp) - 3Dモデル読み込み
- [Dear ImGui](https://github.com/ocornut/imgui) - デバッグUI
- [DirectX 12 Agility SDK](https://devblogs.microsoft.com/directx/directx12agility/) - DirectX 12 API
- (その他あれば追記)

## 3Dモデルのアセットパイプラインについて

このプロジェクトでは、3Dモデルの読み込みに **Assimp** ライブラリを使用しています。

モデルをロードする際、Assimpの `aiProcess_MakeLeftHanded` フラグを利用して、**すべてのモデルデータをDirectXの左手座標系に自動で変換**しています。

### モデル作成者・プログラマー向けのルール

- **BlenderなどのDCCツールからモデルをエクスポートする際は、ツールのデフォルト設定（Y-upの右手座標系）のままエクスポートしてください。**
- 座標系変換のための特別なエクスポート設定は不要です。

このルールにより、エクスポート時の設定ミスを防ぎ、`.obj` や `.gltf` といった異なるフォーマットでも一貫した取り扱いが可能になります。

## テクスチャの命名規則と色空間について

このエンジンは**リニアワークフロー**を採用しています。テクスチャの種類（色データか数値データか）を適切に扱うため、ファイル名に基づいた自動判別を行っています。

### 自動判別のルール

- **カラーデータ (sRGB)**
    - 対象: BaseColor, Diffuse, UI画像など
    - 命名規則: **制約なし**（デフォルトで色として扱われます）
    - 処理: 読み込み時にリニア変換（リニアライズ）が行われます。

- **数値データ (Linear)**
    - 対象: NormalMap, AO, Metallic, Roughnessなど
    - 命名規則: ファイル名に以下のキーワードを含めてください。
        - 法線: `_n`, `normal`
        - AO: `_ao`
        - メタリック: `_m`, `metallic`
        - ラフネス: `_r`, `roughness`
    - 処理: 数値の変換を行わず、生のデータとして読み込みます。

この規則に従うことで、ライティング計算の結果が物理的に正しくなり、意図した通りの質感を得ることができます。

## ライセンス

このプロジェクトは [MIT License](LICENSE.txt) の下で公開されています。
ただし、依存するライブラリについては、それぞれのライセンスに従ってください。
