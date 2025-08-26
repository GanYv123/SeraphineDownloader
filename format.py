import subprocess
import sys
from pathlib import Path
import logging
from concurrent.futures import ThreadPoolExecutor, as_completed

# ------------------- 配置 -------------------
ROOT_DIR = Path(__file__).parent
TOOLS_DIR = ROOT_DIR / "tools"
CLANG_FORMAT = TOOLS_DIR / "clang-format.exe"
IGNORE_FILE = ROOT_DIR / ".clang-format-ignore"
LOG_DIR = ROOT_DIR / "logs"
LOG_DIR.mkdir(exist_ok=True)
LOG_FILE = LOG_DIR / "format_log.txt"

MAX_WORKERS = 4  # 并行线程数

# ------------------- 日志 -------------------
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    handlers=[
        logging.FileHandler(LOG_FILE, mode="a", encoding="utf-8"),  # 追加模式
        logging.StreamHandler()
    ]
)

# ------------------- 忽略规则 -------------------
ignore_dirs = set()
if IGNORE_FILE.exists():
    for line in IGNORE_FILE.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            ignore_dirs.add(line.replace("/", "\\").lower())  # Windows path normalize

# ------------------- 工具检查 -------------------
if not CLANG_FORMAT.exists():
    logging.error(f"clang-format 未找到: {CLANG_FORMAT}")
    sys.exit(1)

# ------------------- 格式化函数 -------------------
def should_ignore(path: Path) -> bool:
    for ign in ignore_dirs:
        if ign in str(path).lower():
            return True
    return False

def format_file(file_path: Path):
    if should_ignore(file_path):
        logging.info(f"跳过: {file_path}")
        return

    try:
        subprocess.run([str(CLANG_FORMAT), "-i", "-style=file", str(file_path)],
                       check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        logging.info(f"格式化成功: {file_path}")
    except subprocess.CalledProcessError as e:
        logging.error(f"格式化失败: {file_path}\n{e.stderr.decode('utf-8', errors='ignore')}")

# ------------------- 遍历目录 -------------------
def walk_and_format(root_dirs):
    futures = []
    with ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        for root in root_dirs:
            for file_path in root.rglob("*"):
                if file_path.suffix.lower() in [".cpp", ".h"]:
                    futures.append(executor.submit(format_file, file_path))

        # 等待所有任务完成
        for future in as_completed(futures):
            pass

# ------------------- 主程序 -------------------
if __name__ == "__main__":
    logging.info("==============================")
    logging.info("开始批量格式化项目代码...")
    logging.info("==============================")

    targets = [ROOT_DIR / "src", ROOT_DIR / "include"]
    walk_and_format(targets)

    logging.info("==============================")
    logging.info("批量格式化完成")
    logging.info("==============================")
