## Guía rápida

Sigue estos pasos en orden para poner a funcionar el dispositivo la primera vez y en sesiones futuras.

### 1) Preparar el entorno
- Requisitos: PlatformIO (extensión de VS Code o `platformio` por CLI), Python 3.
- Conecta el ESP32 por USB.

### 2) Crear tu archivo .env (una sola vez)
Crea `.env` en la raíz del proyecto con tus valores:
```ini
COUNTRY=colombia
STATE=valle
CITY=tulua
MQTT_SERVER=mqtt.tu-dominio.com
MQTT_PORT=8883
MQTT_USER=miuser
MQTT_PASSWORD=supersecreto
WIFI_SSID=MiWiFiInicial
WIFI_PASSWORD=MiPassInicial
ROOT_CA=-----BEGIN CERTIFICATE-----\nMIIF...\n-----END CERTIFICATE-----
```
Notas:
- `ROOT_CA` debe ir en una sola línea usando `\n` como saltos de línea.
- `WIFI_SSID/PASSWORD` son opcionales (se puede configurar luego por el portal del AP).

### 3) Compilar y subir el firmware al ESP32
- Compilar:
```bash
python scripts/build_with_env.py
```
- Subir al dispositivo:
```bash
python scripts/build_with_env.py upload
```

### 4) Configurar el Wi‑Fi (si el equipo es nuevo o fue reseteado)
- El equipo crea un AP: busca una red como `ESP32-Setup-XXXXXX` y conéctate.
- Abre el navegador en `http://192.168.4.1`.
- Completa el formulario con tu SSID y contraseña → Guardar.
- El dispositivo se reinicia y se conecta a tu red. Las credenciales quedan guardadas (persisten tras OTA).

Si necesitas reconfigurar:
- Apaga el equipo, mantén BOOT (GPIO0) presionado 3+ s al encender, y repite los pasos de arriba.

### 5) Verificar que está funcionando
- En el monitor serie verás el SSID y la IP asignada.
- Si tiene pantalla, mostrará el SSID/IP o el estado.

### 6) Desarrollar y subir cambios (sesiones futuras)
- Edita el código.
- Compila y sube de nuevo:
```bash
python scripts/build_with_env.py
python scripts/build_with_env.py upload
```
- No necesitas reconfigurar Wi‑Fi: el dispositivo mantiene las credenciales en NVS.

### 7) Actualización OTA (opcional, vía GitHub)
Para enviar una OTA a los dispositivos suscritos al tópico definido en `src/libota.h` (por defecto `dispositivo/device1/ota`):
- Haz commit y push de tus cambios a `main`:
```bash
git add .
git commit -m "feat: mejora X"
git push origin main
```
- Crea un tag de versión y empújalo (formato `vX.Y.Z`):
```bash
git tag v1.2.0
git push origin v1.2.0
```
- GitHub Actions compila, sube el binario a S3 y publica un mensaje OTA.
- Verifica en GitHub → Actions que aparezca “Mensaje OTA enviado”.

### 8) Problemas comunes y soluciones
- El portal no abre: escribe manualmente `http://192.168.4.1` y desactiva datos móviles.
- No veo el AP: apaga/enciende el equipo o mantén BOOT 3+ s al encender.
- Falla compilación por ROOT_CA: revisa que está en una sola línea con `\n`.
- OTA no llega: revisa que el dispositivo esté conectado a MQTT y suscrito al tópico de OTA, y que los Secrets de GitHub estén bien configurados.

### Referencias (por si quieres detalles)
- `WIFI_SETUP.md`: guía del portal AP de Wi‑Fi.
- `OTA_SETUP.md`: cómo lanzar OTA con tags.
- `SECRETS_SETUP.md`: cómo configurar `.env` y Secrets de GitHub.
