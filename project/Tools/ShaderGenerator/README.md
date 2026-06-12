# AI Shader Generator (Magic Brush Backend)

このディレクトリは、IrufemiEngineの「AI画工のマジックブラシ」機能をバックエンドで支えるPythonサーバーです。
Gemini API (Google) を用いて、エディタ（C++）から送られてきた画像とプロンプトをもとに、HLSLシェーダーコードを自動生成・自己修復します。

## 🚀 環境構築 (別PCで使う場合)
他のPCにリポジトリをクローンした場合は、以下の手順で環境をセットアップしてください。

1. **Pythonの準備**
   - Python 3.10以上がインストールされていること。
   - インストール時に `Add python.exe to PATH` を有効にしてください。

2. **依存ライブラリのインストール**
   コマンドプロンプトでこのディレクトリ（`project/Tools/ShaderGenerator`）を開き、以下のコマンドを実行します。
   ```cmd
   python -m pip install -r requirements.txt
   ```

3. **APIキーの設定**
   このディレクトリに `.env` という名前のファイルを作成し、以下の形式でGoogle AI Studioで取得したAPIキーを貼り付けます。
   ```env
   GEMINI_API_KEY=ここにあなたのAPIキーを貼り付け
   ```
   ※ `.env` ファイルはGitにコミットされないよう除外されています。

## 🎮 使い方 (C++アプリからの操作)
サーバーの起動・停止は、すべてIrufemiEngine (Application_solo) の `TL1Scene` にある **ImGuiパネル** から操作できます。
ターミナルを開いて手動で `python main.py` を実行する必要はありません。

1. エンジンを起動し、ImGuiの「AI Magic Brush」パネルを開く。
2. **Start Server** を押す。（裏で自動的にこのPythonサーバーが起動します）
3. 画像パス(`Image Path`)と要望(`Prompt`)を入力して **Generate Shader** を押す。
4. 生成とエラー修復が完了すると、自動的に画面にエフェクトが反映されます！

## ⚠️ 注意点・よくあるトラブル
- **C++アプリがクラッシュした場合**
  - C++アプリを強制終了（デバッグ停止など）させた場合、裏で動いているPythonサーバーが「ゾンビプロセス」として残ってしまうことがあります。
  - その状態で再度アプリを起動して Start Server を押しても、ポート被り(8000番)によりサーバーが `Stopped` のままになります。
  - **解決策**: C++アプリ上から「Restart Server」を押すか、Windowsのタスクマネージャーから残っている `python.exe` を終了させてください。
- **503 UNAVAILABLE エラーが出る場合**
  - Gemini API（無料枠）のサーバーが混雑しているサインです。数分待ってから再度 Generate を試してください。
- **指示を変えたい場合**
  - このディレクトリにある `system_prompt.txt` の文章を書き換えることで、AIに与える「エンジンの仕様ルール」や「出力の制限」を変更できます。
