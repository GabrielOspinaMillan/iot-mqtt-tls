# Configurar ACL en EMQX usando File (Dashboard)

Esta guía te explica paso a paso cómo configurar el ACL usando el método "File" desde el Dashboard de EMQX de forma sencilla.

## Paso 1: Acceder al Dashboard

1. Abre tu navegador y ve a: `http://localhost:18083` (o la IP de tu servidor EMQX)
2. Inicia sesión con tus credenciales de administrador
3. En el menú lateral izquierdo, ve a **Access Control** → **Authorization**

## Paso 2: Habilitar File ACL

1. En la página de **Authorization**, busca la sección **"Authorization Sources"** o **"Sources"**
2. Haz clic en **"Create"** o **"Add"** para agregar una nueva fuente de autorización
3. Selecciona el tipo: **"File"**
4. Activa el switch **"Enable"** para habilitarlo
5. Guarda la configuración

## Paso 3: Crear o Editar el Archivo ACL

1. En la misma página, busca el campo **"File Path"** o **"Path"**
2. Por defecto suele ser: `etc/acl.conf` o `data/acl.conf`
3. Haz clic en **"Edit"** o **"View"** para abrir el editor de archivos
4. Si no existe el archivo, haz clic en **"Create"** para crearlo

## Paso 4: Copiar y Pegar las Reglas ACL

Copia todo el siguiente contenido y pégalo en el archivo `acl.conf`:

```erlang
%% --- permisos especiales existentes ---
%% "dashboard" puede leer métricas
{allow, {username, "dashboard"}, subscribe, ["$SYS/#"]}.

%% localhost full (útil para debugging/bridge local)
{allow, {ipaddr, "127.0.0.1"}, all, ["$SYS/#", "#"]}.

%% --- equivalentes a tus patterns globales ---
%% Todos los usuarios pueden SUSCRIBIR a .../%u/in
{allow, all, subscribe, ["+/+/+/+/${username}/in"]}.

%% Todos pueden PUBLICAR y SUSCRIBIR a .../%u/out
{allow, all, publish,   ["+/+/+/+/${username}/out"]}.
{allow, all, subscribe, ["+/+/+/+/${username}/out"]}.

%% Opcional: Permitir cualquier device
{allow, {user, "alvaro"}, subscribe, ["dispositivo/+/ota"]}.

%% --- admins ---
%% admin: acceso total
{allow, {username, "admin"}, all, ["#"]}.

%% admin2: write a .../%u/in y read de .../%u/out
{allow, {username, "admin2"}, publish,   ["+/+/+/+/${username}/in"]}.
{allow, {username, "admin2"}, subscribe, ["+/+/+/+/${username}/out"]}.

%% --- bloqueos de seguridad ---
%% nadie se suscribe a $SYS ni al comodín raíz
{deny, all, subscribe, ["$SYS/#", {eq, "#"}]}.

%% --- por defecto: negar todo lo que no matchee ---
{deny, all}.
```

## Paso 5: Personalizar el Usuario

**IMPORTANTE**: Debes cambiar `"alvaro"` por tu usuario MQTT real.

En la línea que dice:
```erlang
{allow, {user, "alvaro"}, subscribe, ["dispositivo/+/ota"]}.
```

Reemplaza `"alvaro"` con el usuario que configuraste en tu código (el valor de `MQTT_USER`).

**Ejemplo**: Si tu usuario es `"juan"`, la línea quedaría:
```erlang
{allow, {user, "juan"}, subscribe, ["dispositivo/+/ota"]}.
```

## Paso 6: Guardar y Aplicar

1. Haz clic en **"Save"** o **"Apply"** para guardar el archivo
2. El Dashboard te pedirá confirmación
3. Haz clic en **"Confirm"** o **"OK"**

## Paso 7: Reiniciar EMQX (si es necesario)

Algunas versiones de EMQX requieren reiniciar para aplicar cambios en el archivo ACL:

1. Ve a **Dashboard** → **System** → **Nodes**
2. Haz clic en el botón **"Restart"** del nodo
3. O reinicia EMQX desde la terminal:
   ```bash
   # Si usas Docker:
   docker restart emqx
   
   # Si usas instalación directa:
   emqx restart
   ```

## Verificación

Para verificar que el ACL funciona:

1. **Conecta tu dispositivo ESP32** y revisa el Serial Monitor
2. Deberías ver: `✓ Suscrito exitosamente al tópico OTA: dispositivo/device1/ota`
3. Si ves `✗ Error al suscribirse al tópico OTA`, revisa:
   - Que el usuario en el ACL coincida con tu `MQTT_USER`
   - Que hayas guardado y reiniciado EMQX

## Explicación Rápida de las Reglas

| Regla | ¿Qué hace? |
|-------|------------|
| `dashboard` | Permite que el usuario "dashboard" vea métricas del sistema |
| `127.0.0.1` | Permite acceso total desde localhost (tu computadora) |
| Todos `+/+/+/+/${username}/in` | Cualquier usuario puede recibir mensajes en su topic de entrada |
| Todos `+/+/+/+/${username}/out` | Cualquier usuario puede publicar y recibir en su topic de salida |
| **`alvaro` → `dispositivo/+/ota`** | **Tu usuario puede recibir actualizaciones OTA** ⭐ |
| `admin` | Usuario admin tiene acceso total |
| `admin2` | Usuario admin2 puede escribir comandos y leer datos |
| Bloquear `$SYS/#` | Nadie puede suscribirse a topics del sistema (excepto dashboard) |
| `{deny, all}` | Niega todo lo demás (regla de seguridad) |

## Troubleshooting

### El dispositivo no puede suscribirse al topic OTA

**Solución:**
1. Verifica que el usuario en el ACL sea exactamente igual al que usas en `MQTT_USER`
2. Revisa que la regla OTA esté antes de la regla `{deny, all}` (al final del archivo)
3. Asegúrate de haber reiniciado EMQX después de guardar el archivo

### No puedo ver el archivo en el Dashboard

**Solución:**
1. Algunos dashboards no tienen editor de archivos integrado
2. En ese caso, edita el archivo directamente en el servidor:
   ```bash
   # Ubicación típica del archivo
   nano /etc/emqx/etc/acl.conf
   # O si usas Docker:
   docker exec -it emqx nano /opt/emqx/etc/acl.conf
   ```

### El archivo no se aplica

**Solución:**
1. Verifica que el File ACL esté habilitado en el Dashboard
2. Reinicia EMQX completamente
3. Revisa los logs de EMQX para ver si hay errores de sintaxis en el archivo

## Notas Finales

- **Orden importa**: Las reglas se evalúan de arriba hacia abajo
- **Primera coincidencia gana**: Si una regla coincide, se aplica y se detiene la evaluación
- **Wildcards**:
  - `+` = un solo nivel (ej: `dispositivo/+/ota`)
  - `#` = múltiples niveles (ej: `dispositivo/#`)
  - `${username}` = se reemplaza con el nombre de usuario del cliente

¡Listo! Tu ACL debería estar configurado correctamente. 🎉
