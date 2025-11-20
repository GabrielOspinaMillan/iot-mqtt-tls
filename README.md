# IoT MQTT TLS (ESP32)

Proyecto base para ESP32 con MQTT seguro (TLS), provisión Wi‑Fi por portal AP, y OTA vía GitHub Actions.

### Quick Start
1) Instala requisitos: PlatformIO (VS Code o CLI) y Python 3.
2) Haz un fork del repositorio en GitHub (tu cuenta) y clona TU fork:
```bash
git clone https://github.com/<tu-usuario>/iot-mqtt-tls.git
cd iot-mqtt-tls
```
3) En tu fork: habilita GitHub Actions (tab Actions → “I understand my workflows…” → Enable) y configura los Secrets si usarás OTA (ver [SECRETS_SETUP.md](SECRETS_SETUP.md)).
4) Crea `.env` (valores de ejemplo):
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
5) Compila y sube al ESP32:
```bash
python scripts/build_with_env.py
python scripts/build_with_env.py upload
```
6) Configura el Wi‑Fi (si es la primera vez):
   - Conéctate al AP `ESP32-Setup-XXXXXX`
   - Abre `http://192.168.4.1`, ingresa SSID y contraseña, Guardar

Listo: el equipo se conecta y guarda credenciales (persisten tras OTA).

### OTA por tags en TU fork (opcional)
- Haz commit y push de tus cambios a `main` de tu fork y crea un tag:
```bash
git add . && git commit -m "feat: cambio" && git push origin main
git tag v1.2.0 && git push origin v1.2.0
```
- El workflow compila, sube el binario a S3 y publica el mensaje OTA al tópico definido en `src/libota.h`.

### Troubleshooting rápido
- Portal no abre: usa `http://192.168.4.1` y desactiva datos móviles.
- No aparece el AP: reinicia o mantén BOOT 3+ s al encender.
- Error ROOT_CA: ponlo en una sola línea con `\n`.
- OTA no llega: revisa Secrets de GitHub y suscripción al tópico OTA.

### Más detalles
- Guía rápida extendida: [GUIA_RAPIDA.md](GUIA_RAPIDA.md)
- Configurar `.env` y Secrets: [SECRETS_SETUP.md](SECRETS_SETUP.md)
- Instrucciones de AP Wi‑Fi: [WIFI_SETUP.md](WIFI_SETUP.md)
- Lanzar OTA con tags: [OTA_SETUP.md](OTA_SETUP.md)

## Licencia

Este proyecto está bajo la licencia MIT. Para más detalles, consulta el archivo [LICENSE](LICENSE).
