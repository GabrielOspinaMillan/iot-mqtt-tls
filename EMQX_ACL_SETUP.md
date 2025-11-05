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

Esta es la forma más fácil y visual de configurar el ACL. El Dashboard de EMQX te permite crear reglas ACL de forma interactiva.

#### Paso 1: Acceder al Dashboard

1. Abre tu navegador y ve a: `http://localhost:18083` (o la IP de tu servidor EMQX)
2. Inicia sesión con tus credenciales de administrador
3. En el menú lateral, ve a **Access Control** → **ACL**

#### Paso 2: Configurar las reglas ACL

En la página de ACL, haz clic en **"Create"** o **"Add Rule"** para crear cada regla. Te explicamos cada campo:

---

#### **Regla 1: Permitir que "dashboard" lea métricas del sistema**

**¿Para qué sirve?**: Permite que el usuario "dashboard" pueda suscribirse a los topics del sistema (`$SYS/#`) para ver métricas y estadísticas.

**Configuración en el Dashboard:**

1. **Action** (Acción): Selecciona `Allow` (Permitir)
   - Esto permite la acción especificada

2. **Principal Type** (Tipo de Principal): Selecciona `Username`
   - Indica que la regla se aplica a un usuario específico

3. **Principal** (Principal): Escribe `dashboard`
   - Este es el nombre de usuario al que se aplica la regla

4. **Permission** (Permiso): Selecciona `Subscribe`
   - El usuario puede suscribirse (recibir) mensajes

5. **Topic** (Tópico): Escribe `$SYS/#`
   - `$SYS/` es el prefijo para topics del sistema
   - `#` es un wildcard que coincide con todos los subtopics
   - Ejemplo: `$SYS/brokers`, `$SYS/clients`, etc.

6. **QoS** (Calidad de Servicio): Selecciona `0, 1, 2` (todos)
   - Permite cualquier nivel de QoS

**¿Qué significa esta regla?**
```
Permite que el usuario "dashboard" se suscriba a cualquier topic que empiece con "$SYS/"
```

---

#### **Regla 2: Permitir acceso completo desde localhost**

**¿Para qué sirve?**: Permite que cualquier conexión desde localhost (127.0.0.1) tenga acceso completo. Útil para debugging y herramientas locales.

**Configuración en el Dashboard:**

1. **Action**: `Allow`

2. **Principal Type**: Selecciona `IP Address`
   - Indica que la regla se aplica a una dirección IP

3. **Principal**: Escribe `127.0.0.1`
   - La dirección IP localhost (tu propia máquina)

4. **Permission**: Selecciona `All`
   - Permite tanto publicar como suscribirse

5. **Topic**: Escribe `$SYS/#,#`
   - `$SYS/#` para topics del sistema
   - `#` para todos los topics (separado por coma)
   - Nota: En algunos dashboards puedes necesitar crear dos reglas separadas

6. **QoS**: `0, 1, 2`

**¿Qué significa esta regla?**
```
Cualquier conexión desde localhost puede hacer lo que quiera (publish y subscribe)
```

---

#### **Regla 3: Todos los usuarios pueden suscribirse a su topic de entrada**

**¿Para qué sirve?**: Permite que cualquier usuario se suscriba a su propio topic de entrada usando el patrón `{COUNTRY}/{STATE}/{CITY}/{client_id}/{username}/in`

**Configuración en el Dashboard:**

1. **Action**: `Allow`

2. **Principal Type**: Selecciona `All`
   - Se aplica a todos los usuarios

3. **Principal**: Deja vacío o selecciona "All"
   - No necesitas especificar un usuario específico

4. **Permission**: `Subscribe`
   - Solo pueden recibir mensajes, no publicar

5. **Topic**: Escribe `+/+/+/+/${username}/in`
   - `+` es un wildcard que coincide con cualquier valor en ese nivel
   - `${username}` es una variable que se reemplaza con el nombre de usuario del cliente
   - Ejemplo: Si el usuario es "alvaro", coincide con `colombia/valle/tulua/ESP32-XXXXXX/alvaro/in`

6. **QoS**: `0, 1, 2`

**¿Qué significa esta regla?**
```
Cualquier usuario puede suscribirse a su propio topic de entrada, independientemente
de su país, estado, ciudad o client_id, siempre que el último nivel antes de "/in" 
sea su nombre de usuario.
```

**Ejemplo de topics que coinciden:**
- `colombia/valle/tulua/ESP32-123456/alvaro/in` ✅ (si el usuario es "alvaro")
- `mexico/cdmx/ciudad/ESP32-789012/juan/in` ✅ (si el usuario es "juan")

---

#### **Regla 4: Todos pueden publicar a su topic de salida**

**¿Para qué sirve?**: Permite que cualquier usuario publique datos a su topic de salida usando el patrón `{COUNTRY}/{STATE}/{CITY}/{client_id}/{username}/out`

**Configuración en el Dashboard:**

1. **Action**: `Allow`

2. **Principal Type**: `All`

3. **Principal**: Deja vacío

4. **Permission**: `Publish`
   - Solo pueden publicar (enviar) mensajes

5. **Topic**: Escribe `+/+/+/+/${username}/out`
   - Similar a la regla anterior, pero termina en `/out`

6. **QoS**: `0, 1, 2`

**¿Qué significa esta regla?**
```
Cualquier usuario puede publicar mensajes a su propio topic de salida.
```

---

#### **Regla 5: Todos pueden suscribirse a su topic de salida**

**¿Para qué sirve?**: Permite que los usuarios también puedan suscribirse a su propio topic de salida (útil para debugging o verificar que los mensajes se publicaron correctamente).

**Configuración en el Dashboard:**

1. **Action**: `Allow`

2. **Principal Type**: `All`

3. **Principal**: Deja vacío

4. **Permission**: `Subscribe`

5. **Topic**: Escribe `+/+/+/+/${username}/out`
   - Mismo patrón que la regla anterior

6. **QoS**: `0, 1, 2`

---

#### **Regla 6: Usuario específico puede suscribirse a topic OTA (CRÍTICA)**

**¿Para qué sirve?**: Esta es la regla MÁS IMPORTANTE para que funcione la actualización OTA. Permite que el usuario "alvaro" (o el que uses) se suscriba a los topics de actualización OTA.

**Configuración en el Dashboard:**

1. **Action**: `Allow`

2. **Principal Type**: Selecciona `Username`
   - Esta regla es específica para un usuario

3. **Principal**: Escribe `alvaro`
   - **IMPORTANTE**: Reemplaza "alvaro" con tu usuario MQTT real (el que está en `MQTT_USER`)

4. **Permission**: `Subscribe`
   - Solo necesita recibir mensajes OTA

5. **Topic**: Escribe `dispositivo/+/ota`
   - `dispositivo/` es el prefijo fijo
   - `+` permite cualquier device (device1, device2, etc.)
   - `/ota` es el sufijo
   - Esto coincide con: `dispositivo/device1/ota`, `dispositivo/device2/ota`, etc.

6. **QoS**: `0, 1, 2`

**¿Qué significa esta regla?**
```
El usuario "alvaro" puede suscribirse a cualquier topic de actualización OTA
que siga el patrón dispositivo/{cualquier_device}/ota
```

**Ejemplo de topics que coinciden:**
- `dispositivo/device1/ota` ✅
- `dispositivo/device2/ota` ✅
- `dispositivo/ESP32-123456/ota` ✅

---

#### **Regla 7: Admin tiene acceso total**

**¿Para qué sirve?**: Permite que el usuario "admin" tenga acceso completo a todos los topics (útil para administración y debugging).

**Configuración en el Dashboard:**

1. **Action**: `Allow`

2. **Principal Type**: `Username`

3. **Principal**: Escribe `admin`

4. **Permission**: `All`
   - Permite publicar y suscribirse

5. **Topic**: Escribe `#`
   - `#` es un wildcard que coincide con TODOS los topics

6. **QoS**: `0, 1, 2`

**⚠️ ADVERTENCIA**: Esta regla da acceso total. Úsala solo para usuarios de administración.

---

#### **Regla 8: Admin2 puede escribir en topics de entrada y leer de salida**

**¿Para qué sirve?**: Permite que "admin2" publique mensajes a los topics de entrada de los usuarios (para enviar comandos) y suscribirse a sus topics de salida (para leer datos).

**Configuración en el Dashboard:**

**Regla 8a - Publicar a topics de entrada:**

1. **Action**: `Allow`
2. **Principal Type**: `Username`
3. **Principal**: `admin2`
4. **Permission**: `Publish`
5. **Topic**: `+/+/+/+/${username}/in`
6. **QoS**: `0, 1, 2`

**Regla 8b - Suscribirse a topics de salida:**

1. **Action**: `Allow`
2. **Principal Type**: `Username`
3. **Principal**: `admin2`
4. **Permission**: `Subscribe`
5. **Topic**: `+/+/+/+/${username}/out`
6. **QoS**: `0, 1, 2`

---

#### **Regla 9: Bloquear suscripciones a $SYS y raíz**

**¿Para qué sirve?**: Previene que usuarios normales se suscriban a topics del sistema o al wildcard raíz por seguridad.

**Configuración en el Dashboard:**

**Regla 9a - Bloquear $SYS:**

1. **Action**: `Deny`
   - Niegue la acción

2. **Principal Type**: `All`

3. **Principal**: Deja vacío

4. **Permission**: `Subscribe`

5. **Topic**: `$SYS/#`

6. **QoS**: `0, 1, 2`

**Regla 9b - Bloquear wildcard raíz:**

1. **Action**: `Deny`
2. **Principal Type**: `All`
3. **Principal**: Deja vacío
4. **Permission**: `Subscribe`
5. **Topic**: `#`
6. **QoS**: `0, 1, 2`

**⚠️ IMPORTANTE**: Las reglas `Deny` deben estar ANTES de las reglas `Allow` generales para que funcionen correctamente. En EMQX, las reglas se evalúan en orden.

---

#### **Regla 10: Negar todo lo demás (regla por defecto)**

**¿Para qué sirve?**: Esta es la regla de seguridad final. Niega todo lo que no haya sido permitido explícitamente por las reglas anteriores.

**Configuración en el Dashboard:**

1. **Action**: `Deny`

2. **Principal Type**: `All`

3. **Principal**: Deja vacío

4. **Permission**: `All`
   - Niega tanto publicar como suscribirse

5. **Topic**: `#`
   - Aplica a todos los topics

6. **QoS**: `0, 1, 2`

**⚠️ IMPORTANTE**: Esta regla debe ser la ÚLTIMA en el orden de evaluación. Si está antes de otras reglas, bloqueará todo.

---

### Orden de las reglas ACL

El orden importa. EMQX evalúa las reglas de arriba hacia abajo y se detiene en la primera que coincide. Orden recomendado:

1. Reglas `Deny` específicas (Reglas 9a, 9b)
2. Reglas `Allow` específicas por usuario (Reglas 1, 6, 7, 8)
3. Reglas `Allow` generales (Reglas 2, 3, 4, 5)
4. Regla `Deny` por defecto (Regla 10)

En el Dashboard, puedes reordenar las reglas usando las flechas ↑↓ o arrastrando.

---

### Guía rápida: Valores para copiar/pegar

Si tu usuario MQTT es **"alvaro"** y usas los valores por defecto del código:

| Regla | Principal | Permission | Topic |
|-------|-----------|------------|-------|
| Dashboard | `dashboard` | Subscribe | `$SYS/#` |
| Localhost | `127.0.0.1` (IP) | All | `$SYS/#,#` |
| Todos - Entrada | `All` | Subscribe | `+/+/+/+/${username}/in` |
| Todos - Salida PUB | `All` | Publish | `+/+/+/+/${username}/out` |
| Todos - Salida SUB | `All` | Subscribe | `+/+/+/+/${username}/out` |
| **OTA (CRÍTICA)** | `alvaro` | Subscribe | `dispositivo/+/ota` |
| Admin | `admin` | All | `#` |
| Admin2 PUB | `admin2` | Publish | `+/+/+/+/${username}/in` |
| Admin2 SUB | `admin2` | Subscribe | `+/+/+/+/${username}/out` |
| Bloquear $SYS | `All` | Subscribe | `$SYS/#` |
| Bloquear raíz | `All` | Subscribe | `#` |
| Denegar todo | `All` | All | `#` |

---

### Notas importantes sobre el Dashboard

1. **Variables disponibles**: En algunos dashboards, `${username}` puede escribirse como `%u` o `%U`. Verifica la documentación de tu versión de EMQX.

2. **Wildcards**:
   - `+` = un solo nivel (ej: `dispositivo/+/ota`)
   - `#` = múltiples niveles (ej: `dispositivo/#`)

3. **Guardar cambios**: Después de crear cada regla, haz clic en **"Save"** o **"Apply"**.

4. **Probar reglas**: Puedes probar las reglas usando un cliente MQTT como `mosquitto_pub` o `mosquitto_sub`.

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
