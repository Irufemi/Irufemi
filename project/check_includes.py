import os
import re

base_dir = r'F:\school\3_0\WP0\WP0\project\IrufemiEngine\Renderer'
fixes_needed = []

for root, dirs, files in os.walk(base_dir):
    for file in files:
        if file.endswith('.h') or file.endswith('.cpp'):
            path = os.path.join(root, file)
            try:
                with open(path, 'r', encoding='utf-8') as f:
                    content = f.readlines()
            except Exception as e:
                try:
                    with open(path, 'r', encoding='mbcs') as f:
                        content = f.readlines()
                except Exception as e:
                    continue
            for i, line in enumerate(content):
                m = re.search(r'#include\s+\"(\.\.[^\"]+)\"', line)
                if m:
                    rel_path = m.group(1)
                    target = os.path.normpath(os.path.join(root, rel_path))
                    if not os.path.exists(target):
                        # Calculate required level
                        # Engine/ or Resource/ should be at F:\school\3_0\WP0\WP0\project\IrufemiEngine
                        # Depth of current file relative to IrufemiEngine
                        rel_to_engine = os.path.relpath(path, r'F:\school\3_0\WP0\WP0\project\IrufemiEngine')
                        depth = len(rel_to_engine.split(os.sep)) - 1
                        correct_rel_path = '../' * depth + rel_path.split('/', 1)[1] if '/' in rel_path else rel_path
                        
                        # Just printing out the error
                        fixes_needed.append(f"{path}:{i+1} : {rel_path} -> NOT FOUND")

with open('scratch_out.txt', 'w', encoding='utf-8') as f:
    f.write('\n'.join(fixes_needed))
