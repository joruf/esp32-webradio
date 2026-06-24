Import("env")

import subprocess
import sys
from os.path import join

project_dir = env.subst("$PROJECT_DIR")
script = join(project_dir, "scripts", "embed_web_assets.py")


def restore_nav_placeholders(source, target, env):
    subprocess.call([sys.executable, script, "restore"], cwd=project_dir)


subprocess.check_call([sys.executable, script, "stitch"], cwd=project_dir)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", restore_nav_placeholders)
