from fastapi import FastAPI, UploadFile, Form
from fastapi.responses import JSONResponse
import uvicorn
import os
import io
from PIL import Image
from google import genai
from google.genai import types
from pydantic import BaseModel

app = FastAPI(title="AI Magic Brush Backend")

@app.get("/")
def read_root():
    return {"status": "ok", "message": "AI Magic Brush server is running."}

@app.post("/generate")
async def generate_shader(
    image: UploadFile,
    prompt: str = Form(...)
):
    """
    C++のTL1Scene（エディタ）から画像とプロンプトを受け取り、
    Geminiに送信してHLSLを生成するためのエンドポイント
    """
    try:
        file_content = await image.read()
        pil_image = Image.open(io.BytesIO(file_content))
        
        # システムプロンプトの読み込み
        system_instruction_text = ""
        prompt_path = os.path.join(os.path.dirname(__file__), "system_prompt.txt")
        if os.path.exists(prompt_path):
            with open(prompt_path, "r", encoding="utf-8") as f:
                system_instruction_text = f.read()
                
        # API呼び出し (GEMINI_API_KEY環境変数が必要)
        client = genai.Client()
        
        response = client.models.generate_content(
            model='gemini-2.5-pro',
            contents=[pil_image, prompt],
            config=types.GenerateContentConfig(
                system_instruction=system_instruction_text,
                temperature=0.2
            )
        )
        
        hlsl_code = response.text
        # マークダウンのコードブロックを取り除く
        if "```hlsl" in hlsl_code:
            hlsl_code = hlsl_code.split("```hlsl")[1].split("```")[0].strip()
        elif "```" in hlsl_code:
            hlsl_code = hlsl_code.split("```")[1].split("```")[0].strip()
        
        return JSONResponse(content={
            "status": "success",
            "hlsl": hlsl_code
        })

    except Exception as e:
        return JSONResponse(status_code=500, content={
            "status": "error",
            "message": str(e)
        })

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
                
        client = genai.Client()
        
        fix_prompt = f"""
以下のHLSLシェーダーコードをコンパイルしたところ、エラーが発生しました。
エラーの内容を元に、コードを修正して完全なHLSLコードだけを出力してください。

【コンパイルエラー】
{request.error_log}

【元のコード】
```hlsl
{request.code}
```
"""
        response = client.models.generate_content(
            model='gemini-2.5-pro',
            contents=[fix_prompt],
            config=types.GenerateContentConfig(
                system_instruction=system_instruction_text,
                temperature=0.2
            )
        )
        
        hlsl_code = response.text
        if "```hlsl" in hlsl_code:
            hlsl_code = hlsl_code.split("```hlsl")[1].split("```")[0].strip()
        elif "```" in hlsl_code:
            hlsl_code = hlsl_code.split("```")[1].split("```")[0].strip()
            
        return JSONResponse(content={
            "status": "success",
            "hlsl": hlsl_code
        })
    except Exception as e:
        return JSONResponse(status_code=500, content={
            "status": "error",
            "message": str(e)
        })

if __name__ == "__main__":
    # ローカルホストの8000番ポートでサーバーを起動
    uvicorn.run(app, host="127.0.0.1", port=8000)
