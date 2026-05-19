import os
import re

def fix_file(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Could not read {filepath}: {e}")
        return
        
    # Fix the `r`n literal bug
    content = content.replace("`r`n", "\n")
    
    # Fix any remaining <<<<<<< HEAD blocks
    pattern = re.compile(r'<<<<<<< HEAD\n(.*?)\n=======\n(.*?)\n>>>>>>> [^\n]*\n', re.DOTALL)
    content = pattern.sub(r'\1\n\2\n', content)
    
    # Fix any <<<<<<< HEAD blocks with \r\n
    pattern2 = re.compile(r'<<<<<<< HEAD\r\n(.*?)\r\n=======\r\n(.*?)\r\n>>>>>>> [^\r\n]*\r\n', re.DOTALL)
    content = pattern2.sub(r'\1\n\2\n', content)

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

for root, dirs, files in os.walk('.'):
    if '.git' in root: continue
    for f in files:
        if f.endswith('.vcxproj') or f.endswith('.filters') or f.endswith('.cpp') or f.endswith('.h') or f.endswith('.md'):
            fix_file(os.path.join(root, f))
