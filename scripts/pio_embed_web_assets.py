Import("env")

import subprocess
import sys
from os.path import join

project_dir = env.subst("$PROJECT_DIR")
script = join(project_dir, "scripts", "embed_web_assets.py")

subprocess.check_call([sys.executable, script, "generate"], cwd=project_dir)
