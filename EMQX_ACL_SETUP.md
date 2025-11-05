# Configuración de ACL para EMQX

Este documento explica cómo configurar el Access Control List (ACL) en EMQX para que el dispositivo IoT pueda funcionar correctamente.

## Topics que usa el dispositivo

El dispositivo utiliza los siguientes topics:

1. **Topics dinámicos** (construidos automáticamente):
   - **Publicación (PUB)**: `{COUNTRY}/{STATE}/{CITY}/{client_id}/{mqtt_user}/out`
   - **Suscripción (SUB)**: `{COUNTRY}/{STATE}/{CITY}/{client_id}/{mqtt_user}/in`

2. **Topic fijo OTA**:
   - **Suscripción**: `dispositivo/device1/ota`

### Ejemplo de topics con valores por defecto

Si usas los valores por defecto del código:
- `COUNTRY = "colombia"`
- `STATE = "valle"`
- `CITY = "tulua"`
- `mqtt_user = "alvaro"`
- `client_id = "ESP32-XXXXXX"` (MAC address del dispositivo)

Los topics serían:
- PUB: `colombia/valle/tulua/ESP32-XXXXXX/alvaro/out`
- SUB: `colombia/valle/tulua/ESP32-XXXXXX/alvaro/in`
- OTA: `dispositivo/device1/ota`

## Configuración ACL en EMQX

### Opción 1: ACL usando Base de Datos (MySQL/PostgreSQL)

Si usas una base de datos para almacenar ACLs, puedes usar estas consultas SQL:

#### Para tabla MySQL/PostgreSQL:

```sql
-- Permitir publicar a su propio topic de salida
INSERT INTO mqtt_acl (allow, ipaddr, username, clientid, access, topic) VALUES
(1, NULL, 'alvaro', NULL, 2, '{COUNTRY}/{STATE}/{CITY}/+/alvaro/out');

-- Permitir suscribirse a su propio topic de entrada
INSERT INTO mqtt_acl (allow, ipaddr, username, clientid, access, topic) VALUES
(1, NULL, 'alvaro', NULL, 1, '{COUNTRY}/{STATE}/{CITY}/+/alvaro/in');

-- Permitir suscribirse al topic OTA (crítico para recibir actualizaciones)
INSERT INTO mqtt_acl (allow, ipaddr, username, clientid, access, topic) VALUES
(1, NULL, 'alvaro', NULL, 1, 'dispositivo/device1/ota');

-- Permitir suscribirse con wildcard para cualquier device1
INSERT INTO mqtt_acl (allow, ipaddr, username, clientid, access, topic) VALUES
(1, NULL, 'alvaro', NULL, 1, 'dispositivo/+/ota');
```

**Nota**: Reemplaza `{COUNTRY}`, `{STATE}`, `{CITY}` con tus valores reales (ej: `colombia`, `valle`, `tulua`).

### Opción 2: ACL usando archivo de configuración (emqx.conf)

Si prefieres usar archivo de configuración, agrega estas reglas en `emqx.conf`:

```hcl
authorization {
  sources = [
    {
      type = file
      enable = true
    }
  ]
}

authorization.sources.1.type = file
authorization.sources.1.path = "etc/acl.conf"
```

Y en el archivo `etc/acl.conf`:

```erlang
%% Usuario 'alvaro' puede publicar a su topic de salida
{allow, {user, "alvaro"}, publish, ["colombia/valle/tulua/+/alvaro/out"]}.

%% Usuario 'alvaro' puede suscribirse a su topic de entrada
{allow, {user, "alvaro"}, subscribe, ["colombia/valle/tulua/+/alvaro/in"]}.

%% Usuario 'alvaro' puede suscribirse al topic OTA (CRÍTICO)
{allow, {user, "alvaro"}, subscribe, ["dispositivo/device1/ota"]}.

%% Opcional: Permitir cualquier device en el topic OTA
{allow, {user, "alvaro"}, subscribe, ["dispositivo/+/ota"]}.
```

### Opción 3: ACL usando Dashboard de EMQX (Recomendado)

1. Accede al Dashboard de EMQX (http://localhost:18083)

2. Ve a **Authentication & Authorization** → **ACL**

3. Crea una nueva regla ACL con estos permisos:

   **Regla 1: Publicar a topic de salida**
   - **Action**: Allow
   - **Principal**: Username = `alvaro` (o el usuario que uses)
   - **Permission**: Publish
   - **Topic**: `colombia/valle/tulua/+/alvaro/out` (ajusta según tus valores)
   - **QoS**: 0, 1, 2

   **Regla 2: Suscribirse a topic de entrada**
   - **Action**: Allow
   - **Principal**: Username = `alvaro`
   - **Permission**: Subscribe
   - **Topic**: `colombia/valle/tulua/+/alvaro/in`
   - **QoS**: 0, 1, 2

   **Regla 3: Suscribirse a topic OTA (MUY IMPORTANTE)**
   - **Action**: Allow
   - **Principal**: Username = `alvaro`
   - **Permission**: Subscribe
   - **Topic**: `dispositivo/device1/ota`
   - **QoS**: 0, 1, 2

   **Regla 4 (Opcional): Wildcard para OTA**
   - **Action**: Allow
   - **Principal**: Username = `alvaro`
   - **Permission**: Subscribe
   - **Topic**: `dispositivo/+/ota`
   - **QoS**: 0, 1, 2

### Opción 4: ACL usando HTTP API de EMQX

Puedes crear las reglas ACL usando la API REST de EMQX:

```bash
# Permitir publicar a topic de salida
curl -X POST 'http://localhost:8081/api/v5/authorization/sources' \
  -H 'Content-Type: application/json' \
  -d '{
    "type": "built_in_database",
    "enable": true
  }'

# Crear regla para publicar (requiere usar el formato correcto según tu versión de EMQX)
```

## Verificación

Para verificar que el ACL está funcionando correctamente:

1. **Conecta el dispositivo** y revisa el Serial Monitor:
   - Deberías ver: `✓ Suscrito exitosamente al tópico OTA: dispositivo/device1/ota`
   - Si ves `✗ Error al suscribirse al tópico OTA`, el ACL está bloqueando la suscripción

2. **Publica un mensaje de prueba** al topic OTA desde un cliente MQTT:
   ```bash
   mosquitto_pub -h tu-servidor-emqx -p 1883 -u alvaro -P tu-password \
     -t "dispositivo/device1/ota" \
     -m '{"url":"https://example.com/firmware.bin","version":"1.0.0"}'
   ```

3. **Revisa el Serial Monitor del dispositivo**:
   - Deberías ver: `*** CALLBACK MQTT DISPARADO ***`
   - Si no aparece, el ACL está bloqueando la recepción del mensaje

## Troubleshooting

### El dispositivo no recibe mensajes en el topic OTA

**Posibles causas:**

1. **ACL no permite suscripción al topic OTA**
   - Verifica que existe una regla que permita suscribirse a `dispositivo/device1/ota`
   - Asegúrate de que el usuario tiene permisos de Subscribe

2. **Topic no coincide exactamente**
   - Verifica que el topic que publicas es exactamente `dispositivo/device1/ota`
   - No debe tener espacios adicionales ni caracteres especiales

3. **Usuario sin permisos**
   - Verifica que el usuario `mqtt_user` tiene permisos para ese topic
   - Revisa que el usuario existe y está autenticado correctamente

4. **Orden de reglas ACL**
   - EMQX evalúa las reglas en orden
   - Si hay una regla "deny" antes de la "allow", se bloqueará
   - Asegúrate de que las reglas "allow" están antes de las "deny"

### Ver logs de EMQX

Para ver qué está pasando con el ACL:

```bash
# Ver logs de EMQX
tail -f /var/log/emqx/emqx.log

# O si usas Docker
docker logs -f emqx
```

Busca mensajes relacionados con:
- `authorization` (evaluación de ACL)
- `subscribe` (intentos de suscripción)
- `publish` (intentos de publicación)

## Ejemplo completo de ACL para un usuario

```erlang
%% Archivo: etc/acl.conf

%% Usuario 'alvaro' puede publicar datos de sensores
{allow, {user, "alvaro"}, publish, ["colombia/valle/tulua/+/alvaro/out"]}.

%% Usuario 'alvaro' puede recibir comandos/alertas
{allow, {user, "alvaro"}, subscribe, ["colombia/valle/tulua/+/alvaro/in"]}.

%% Usuario 'alvaro' puede recibir actualizaciones OTA (CRÍTICO)
{allow, {user, "alvaro"}, subscribe, ["dispositivo/device1/ota"]}.
{allow, {user, "alvaro"}, subscribe, ["dispositivo/+/ota"]}.

%% Denegar todo lo demás (opcional, por seguridad)
{deny, all}.
```

## Notas importantes

1. **El topic OTA es crítico**: Si el ACL no permite suscribirse a `dispositivo/device1/ota`, el dispositivo **nunca recibirá** los mensajes de actualización OTA, incluso si publicas correctamente.

2. **Wildcards**: Usa `+` para un nivel de topic y `#` para múltiples niveles:
   - `dispositivo/+/ota` → coincide con `dispositivo/device1/ota`, `dispositivo/device2/ota`, etc.
   - `dispositivo/#` → coincide con cualquier subtopic bajo `dispositivo/`

3. **QoS**: Asegúrate de que el ACL permite QoS 1 (que es el que usa el código ahora).

4. **Reinicia EMQX** después de cambiar el archivo `acl.conf`:
   ```bash
   emqx restart
   # O si usas Docker:
   docker restart emqx
   ```
