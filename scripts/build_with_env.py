#!/usr/bin/env python3
"""
Script simple para invocar PlatformIO. La inyección de defines desde
.env/entorno la hace scripts/add_env_defines.py (extra_scripts).

Uso:
  python scripts/build_with_env.py            # compila
  python scripts/build_with_env.py upload     # compila y sube
"""
import os
import sys
import subprocess
def build(target: str = 'run'):
    cmd = ['pio', 'run', '-e', 'esp32dev']
    if target == 'upload':
        cmd.extend(['-t', 'upload'])
    print("Ejecutando:", ' '.join(cmd))
    return subprocess.run(cmd)

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'upload':
        sys.exit(build('upload').returncode)
    else:
        sys.exit(build('run').returncode)