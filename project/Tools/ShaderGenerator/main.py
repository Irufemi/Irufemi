from fastapi import FastAPI, UploadFile, Form
from fastapi.responses import PlainTextResponse
import uvicorn
import os
import io
import time
import random
from PIL import Image
from google import genai
from google.genai import types
from pydantic import BaseModel
from dotenv import load_dotenv

# .envファイルから環境変数を読み込む
load_dotenv()

app = FastAPI(title="AI Magic Brush Backend")

class GenerateRequest(BaseModel):
    prompt: str
    image_path: str

@app.post("/generate")
async def generate_shader(request: GenerateRequest):
    """
    C++のTL1Scene（エディタ）から画像パスとプロンプトを受け取り、
    Geminiに送信してHLSLを生成するためのエンドポイント
    """
    try:
        if not os.path.exists(request.image_path):
            raise Exception(f"Image not found at path: {request.image_path}")
            
        pil_image = Image.open(request.image_path)
        
        # システムプロンプトの読み込み
        system_instruction_text = ""
        prompt_path = os.path.join(os.path.dirname(__file__), "system_prompt.txt")
        if os.path.exists(prompt_path):
            with open(prompt_path, "r", encoding="utf-8") as f:
                system_instruction_text = f.read()
                
        # API呼び出し (GEMINI_API_KEY環境変数が必要)
        api_key = os.getenv("GEMINI_API_KEY")
        if not api_key:
            raise Exception("GEMINI_API_KEY is not set in .env file or environment variables.")
        client = genai.Client(api_key=api_key)
        
        # APIキャッシュ回避だけでなく、AIに「違うものを出せ」と強烈に指示する
        unique_prompt = f"ユーザーからのリクエスト: {request.prompt}\n\n【AIへの追加指示】\n同じリクエストであっても、絶対に前回とは異なるアプローチ（使用する数学関数、色の組み合わせ、空間の歪ませ方など）を採用してください。\nランダムシード [ {time.time()}_{random.randint(0, 10000)} ] を基準にして、今回はどのような斬新なビジュアルにするか全く新しい構成でHLSLコードを記述してください。\nマンネリ化した同じパターンの出力は避けてください。"
        
        response = client.models.generate_content(
            model='gemini-2.5-flash',
            contents=[pil_image, unique_prompt],
            config=types.GenerateContentConfig(
                system_instruction=system_instruction_text,
                temperature=0.8
            )
        )
        
        hlsl_code = response.text
        if "```hlsl" in hlsl_code:
            hlsl_code = hlsl_code.split("```hlsl")[1].split("```")[0].strip()
        elif "```" in hlsl_code:
            hlsl_code = hlsl_code.split("```")[1].split("```")[0].strip()
        
        return PlainTextResponse(content=hlsl_code)

    except Exception as e:
        return PlainTextResponse(status_code=500, content="ERROR: " + str(e))

class FixRequest(BaseModel):
    error_log: str
    code: str

@app.post("/fix_error")
async def fix_error(request: FixRequest):
    """
    C++エンジン側で発生したコンパイルエラーを修復するエンドポイント
    """
    try:
        system_instruction_text = ""
        prompt_path = os.path.join(os.path.dirname(__file__), "system_prompt.txt")
        if os.path.exists(prompt_path):
            with open(prompt_path, "r", encoding="utf-8") as f:
                system_instruction_text = f.read()
                
        # API呼び出し
        api_key = os.getenv("GEMINI_API_KEY")
        if not api_key:
            raise Exception("GEMINI_API_KEY is not set in .env file or environment variables.")
        client = genai.Client(api_key=api_key)
        
        fix_prompt = f"""
以下のHLSLシェーダーコードをコンパイルしたところ、エラーが発生しました。
エラーの内容を元に、コードを修正して完全なHLSLコードだけを出力してください。
【重要】ピクセルシェーダーのエントリーポイント関数名は、必ず `main` にしてください。(`ps_main`等は不可)

【コンパイルエラー】
{request.error_log}

【元のコード】
```hlsl
{request.code}
```
"""
        response = client.models.generate_content(
            model='gemini-2.5-flash',
            contents=[fix_prompt],
            config=types.GenerateContentConfig(
                system_instruction=system_instruction_text,
                temperature=0.8
            )
        )
        
        hlsl_code = response.text
        if "```hlsl" in hlsl_code:
            hlsl_code = hlsl_code.split("```hlsl")[1].split("```")[0].strip()
        elif "```" in hlsl_code:
            hlsl_code = hlsl_code.split("```")[1].split("```")[0].strip()
            
        return PlainTextResponse(content=hlsl_code)

    except Exception as e:
        return PlainTextResponse(status_code=500, content="ERROR: " + str(e))

if __name__ == "__main__":
    # ローカルホストの8000番ポートでサーバーを起動
    uvicorn.run(app, host="127.0.0.1", port=8000)
