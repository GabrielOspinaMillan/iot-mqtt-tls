Import("env")

from pathlib import Path
import os

# Variables que aceptamos
KNOWN = [
    'COUNTRY','STATE','CITY',
    'MQTT_SERVER','MQTT_PORT','MQTT_USER','MQTT_PASSWORD',
    'WIFI_SSID','WIFI_PASSWORD','ROOT_CA'
]


def load_dotenv(dotenv_path: Path):
    vals = {}
    if not dotenv_path.exists():
        return vals
    for line in dotenv_path.read_text(encoding="utf-8").splitlines():
        s = line.strip()
        if not s or s.startswith('#'):
            continue
        if '=' not in s:
            continue
        k, v = s.split('=', 1)
        k = k.strip()
        v = v.strip()
        if (v.startswith('"') and v.endswith('"')) or (v.startswith("'") and v.endswith("'")):
            v = v[1:-1]
        vals[k] = v
    return vals


root = Path(env['PROJECT_DIR'])
file_vars = load_dotenv(root / '.env')
os_vars = {k: os.environ[k] for k in KNOWN if k in os.environ}

# Merge: entorno del sistema sobreescribe al .env
vals = dict(file_vars)
vals.update(os_vars)

# Construir CPPDEFINES seguros
cpp_defines = []
for k in KNOWN:
    if k not in vals or vals[k] == '':
        continue
    v = vals[k]
    if k == 'MQTT_PORT':
        try:
            cpp_defines.append((k, int(v)))
        except ValueError:
            # Si no es entero, pásalo como string
            cpp_defines.append((k, v))
    else:
        # Pasar como string literal C (PlatformIO se encarga del quoting)
        cpp_defines.append((k, v))

if cpp_defines:
    env.Append(CPPDEFINES=cpp_defines)
    print("[add_env_defines] Defines aplicados:", [d[0] if isinstance(d, tuple) else d for d in cpp_defines])
else:
    print("[add_env_defines] Sin defines desde .env/entorno; se usarán defaults del código.")
