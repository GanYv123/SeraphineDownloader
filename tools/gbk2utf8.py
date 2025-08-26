import os

# 要转换的目录
dirs = ["include", "src"]

# 需要处理的文件扩展名
exts = [".h", ".hpp", ".c", ".cpp"]

def convert_to_utf8_bom(file_path):
    try:
        with open(file_path, "r", encoding="gb2312", errors="ignore") as f:
            content = f.read()
        # 写入 UTF-8 带 BOM
        with open(file_path, "w", encoding="utf-8-sig") as f:
            f.write(content)
        print(f"[OK] {file_path}")
    except Exception as e:
        print(f"[FAIL] {file_path} : {e}")

for d in dirs:
    for root, _, files in os.walk(d):
        for name in files:
            if any(name.lower().endswith(ext) for ext in exts):
                file_path = os.path.join(root, name)
                convert_to_utf8_bom(file_path)
