import os

yaml_content = """name: Gemini Push Reviewer

on:
  push:
    branches:
      - '**'

jobs:
  review:
    runs-on: ubuntu-latest
    permissions:
      contents: write

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Install dependencies
        run: |
          python -m pip install --upgrade pip
          pip install google-genai requests

      - name: Run Reviewer Script
        env:
          GEMINI_API_KEY: ${{ secrets.GEMINI_API_KEY }}
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
          COMMIT_SHA: ${{ github.sha }}
          REPO_NAME: ${{ github.repository }}
        run: |
          python .github/scripts/push_reviewer.py
"""

py_content = """import os
import sys
import requests
from google import genai
from google.genai import types

def get_commit_diff(repo, commit_sha, token):
    url = f"https://api.github.com/repos/{repo}/commits/{commit_sha}"
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github.v3.diff"
    }
    response = requests.get(url, headers=headers)
    if response.status_code != 200:
        print(f"Failed to fetch commit diff: {response.status_code}")
        sys.exit(1)
    return response.text

def post_commit_comment(repo, commit_sha, token, body):
    url = f"https://api.github.com/repos/{repo}/commits/{commit_sha}/comments"
    headers = {
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github.v3+json"
    }
    data = {"body": body}
    response = requests.post(url, headers=headers, json=data)
    if response.status_code != 201:
        print(f"Failed to post comment: {response.status_code}")
        print(response.text)
        sys.exit(1)

def main():
    api_key = os.getenv("GEMINI_API_KEY")
    github_token = os.getenv("GITHUB_TOKEN")
    commit_sha = os.getenv("COMMIT_SHA")
    repo = os.getenv("REPO_NAME")

    if not all([api_key, github_token, commit_sha, repo]):
        print("Missing required environment variables.")
        sys.exit(1)

    # 1. 差分の取得
    diff_text = get_commit_diff(repo, commit_sha, github_token)
    
    if not diff_text.strip():
        print("No diff found. Exiting.")
        sys.exit(0)

    # 2. Geminiによるレビュー
    system_prompt = '''あなたは、IrufemiEngine という C++ ゲームエンジンのエキスパート開発者であり、厳格なコードレビュアーです。
与えられた git diff（Pushされたコミットの差分）を読み、以下のプロジェクトルールに従ってレビューを行ってください。
指摘事項がある場合は、どのファイルのどの箇所かを含め、修正案とともに日本語で簡潔にまとめてください。
問題が全くない場合は「LGTM! プロジェクトルールへの違反は見当たりません。」と出力してください。

【プロジェクトルール】
1. Architecture (アーキテクチャ)
   - `IrufemiEngine/` ディレクトリ配下のファイルには、特定のゲームやシーンに依存する処理・アクター・固有データを絶対に含めないこと。
   - ゲーム固有のロジックやキャラクター制御は、必ず `Application/` (または `Application_solo/`) ディレクトリ配下に記述すること。
2. Modern C++
   - C++17 または C++20 基準のモダンな構文を積極的に使用すること（例: `std::clamp`, `<filesystem>`, range-based for 等）。
3. Safety (安全性)
   - メモリリークを防ぐため、生ポインタ（Raw Pointer）の新規使用は極力避け、スマートポインタ（`std::unique_ptr`, `std::shared_ptr` 等）を優先すること。
4. DirectX
   - DirectXのCOMオブジェクトを扱う際は、必ず `Microsoft::WRL::ComPtr` を使用すること。
   - DirectXのAPI呼び出し時は `HRESULT` の戻り値を必ずチェックし、適切なエラーハンドリング（アサート等）を含めること。
5. Coding Standards & Naming (コーディング規約)
   - インクルードガードには `#pragma once` を使用すること。
   - ヘッダーへの注釈は「Doxygen形式」で記述すること。
   - メンバ変数に `m_` などのプレフィックスをつけるスタイルは厳禁。代わりにキャメルケースの末尾にアンダーバーをつけること（例：`variableName_`）。
6. Leverage Existing Systems (既存システムの活用)
   - 既存の機能で代替できそうな冗長な処理があれば指摘すること。

【出力フォーマット】
マークダウン形式で、レビューコメントのみを出力してください。'''
    
    prompt = f"以下の git diff をレビューしてください:\\n\\n```diff\\n{diff_text}\\n```"
    
    try:
        client = genai.Client(api_key=api_key)
        response = client.models.generate_content(
            model='gemini-2.5-flash',
            contents=prompt,
            config=types.GenerateContentConfig(
                system_instruction=system_prompt,
            )
        )
        review_result = response.text
    except Exception as e:
        print(f"Gemini API Error: {e}")
        sys.exit(1)

    # 3. 結果をコミットにコメント
    comment_body = f"## 🤖 Gemini Push Review\\n\\n{review_result}"
    post_commit_comment(repo, commit_sha, github_token, comment_body)

if __name__ == '__main__':
    main()
"""

with open(r"F:\school\3_0\WP0\WP0\.github\workflows\gemini-push-review.yml", "w", encoding="utf-8") as f:
    f.write(yaml_content)
with open(r"F:\school\3_0\WP0\WP0\.github\scripts\push_reviewer.py", "w", encoding="utf-8") as f:
    f.write(py_content)
