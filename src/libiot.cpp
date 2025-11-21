/*
 * The MIT License
 *
 * Copyright 2024 Alvaro Salazar <alvaro@denkitronik.com>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <libiot.h>
#include <SHTSensor.h>
#include <libota.h>
#include <libstorage.h>
#include <ArduinoJson.h> // Incluir la librería ArduinoJson

// Versión del firmware (debe coincidir con main.cpp)
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "v1.1.1"
#endif

//#define PRINT
#ifdef PRINT
#define PRINTD(x, y) Serial.println(x, y)
#define PRINTLN(x) Serial.println(x)
#define PRINT(x) Serial.print(x)
#else
#define PRINTD(x, y)
#define PRINTLN(x)
#define PRINT(x)
#endif

SHTSensor sht;     //Sensor SHT21
String alert = ""; //Mensaje de alerta
extern const char * client_id;  //ID del cliente MQTT

PulseOximeter pox;  // Sensor MAX30100
MPU6050 mpu;        // Sensor MPU6050

/**
 * Consulta y guarda el tiempo actual con servidores SNTP.
 */
time_t setTime() {
  //Sincroniza la hora del dispositivo con el servidor SNTP (Simple Network Time Protocol)
  Serial.print("Ajustando el tiempo usando SNTP");
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov"); //Configura la zona horaria y los servidores SNTP
  now = time(nullptr);              //Obtiene la hora actual
  while (now < 1700000000) {        //Espera a que el tiempo sea mayor a 1700000000 (1 de enero de 2024)
    delay(500);                     //Espera 500ms antes de volver a intentar obtener la hora
    Serial.print(".");
    now = time(nullptr);            //Obtiene la hora actual
  }
  Serial.println(" hecho!");
  struct tm timeinfo;               //Estructura que almacena la información de la hora
  gmtime_r(&now, &timeinfo);        //Obtiene la hora actual
  Serial.print("Tiempo actual: ");  //Una vez obtiene la hora, imprime en el monitor el tiempo actual
  Serial.print(asctime(&timeinfo));
  return now;
}

// Variable para debugging periódico
static unsigned long lastMQTTDebug = 0;
static const unsigned long MQTT_DEBUG_INTERVAL = 30000; // 30 segundos

/**
 * Conecta el dispositivo con el bróker MQTT usando
 * las credenciales establecidas.
 * Si ocurre un error lo imprime en la consola.
 */
void checkMQTT() {
  if (!client.connected()) {
    reconnect();
  }
  // Procesa mensajes MQTT entrantes (esto es crítico para recibir mensajes)
  // IMPORTANTE: client.loop() debe llamarse frecuentemente para recibir mensajes
  bool loopResult = client.loop();
  if (!loopResult && client.connected()) {
    // Si loop() retorna false pero estamos conectados, podría haber un problema
    Serial.println("⚠ client.loop() retornó false (podría indicar problema de conexión)");
  }
  
  // Debug periódico cada 30 segundos
  unsigned long now = millis();
  if (now - lastMQTTDebug > MQTT_DEBUG_INTERVAL) {
    lastMQTTDebug = now;
    Serial.println("=== Healthcheck MQTT (cada 30s) ===");
    Serial.print("Conectado: ");
    Serial.println(client.connected() ? "✅UP" : "❌DOWN");
  }
}

/**
 * Adquiere la dirección MAC del dispositivo y la retorna en formato de cadena.
 */
String getMacAddress() {
  uint8_t mac[6];
  char macStr[18];
  WiFi.macAddress(mac);
  snprintf(macStr, sizeof(macStr), "ESP32-%02X%02X%02X%02X%02X%02X", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}


/**
 * Función que se ejecuta cuando se establece conexión con el servidor MQTT
 */
void reconnect() {
  while (!client.connected()) { //Mientras no esté conectado al servidor MQTT
    Serial.println("=== Intentando conectar a MQTT ===");
    Serial.print("Servidor: ");
    Serial.println(mqtt_server);
    Serial.print("Puerto: ");
    Serial.println(mqtt_port);
    Serial.print("Usuario: ");
    Serial.println(mqtt_user);
    Serial.print("Client ID: ");
    Serial.println(client_id);
    Serial.print("Conectando...");
    if (client.connect(client_id, mqtt_user, mqtt_password)) { //Intenta conectarse al servidor MQTT
      Serial.println(" ✓ CONECTADO");
      
      // CRÍTICO: Reconfigurar el callback después de reconectar
      client.setCallback(receivedCallback);
      Serial.println("✓ Callback reconfigurado después de reconexión");
      
      // Imprimir versión del firmware al conectar (usar versión guardada)
      String firmwareVersion = getFirmwareVersion();
      Serial.print("Firmware: ");
      Serial.println(firmwareVersion);
      
      Serial.println("=== Suscripciones MQTT ===");
      // Se suscribe al tópico de suscripción con QoS 1
      bool subResult = client.subscribe(MQTT_TOPIC_SUB, 1);
      if (subResult) {
        Serial.println("✓ Suscrito exitosamente a " + String(MQTT_TOPIC_SUB));
      } else {
        Serial.println("✗ Error al suscribirse a " + String(MQTT_TOPIC_SUB));
      }
      
      // Procesar mensajes para confirmar suscripciones
      client.loop();
      delay(100); // Dar tiempo para procesar
      
      setupOTA(client); //Configura la funcionalidad OTA
      
      // Procesar mensajes nuevamente después de suscribirse a OTA
      client.loop();
      delay(100);
      
      Serial.println("==========================");
      Serial.println("Listo para recibir mensajes MQTT");
    } else {
      Serial.println(" ✗ FALLÓ");
      Serial.println("Problema con la conexión, revise los valores de las constantes MQTT");
      int state = client.state();
      Serial.print("Código de error = ");
      alert = "MQTT error: " + String(state);
      Serial.println(state);
      if ( client.state() == MQTT_CONNECT_UNAUTHORIZED ) ESP.deepSleep(0);
      delay(5000); // Espera 5 segundos antes de volver a intentar
    }
  }
}


/**
 * Función setupIoT que configura el certificado raíz, el servidor MQTT y el puerto
 */
void setupIoT() {
  Wire.begin();                 //Inicializa el bus I2C: (SDA, SCL)
  espClient.setCACert(root_ca); //Configura el certificado raíz de la autoridad de certificación
  client.setServer(mqtt_server, mqtt_port);   //Configura el servidor MQTT y el puerto seguro
  
  // Configurar buffer más grande para mensajes grandes (por defecto es 256 bytes)
  client.setBufferSize(1024);
  
  client.setCallback(receivedCallback);       //Configura la función que se ejecutará cuando lleguen mensajes a la suscripción
  Serial.println("=== Configuración MQTT ===");
  Serial.print("Servidor MQTT: ");
  Serial.println(mqtt_server);
  Serial.print("Puerto MQTT: ");
  Serial.println(mqtt_port);
  Serial.print("Usuario MQTT: ");
  Serial.println(mqtt_user);
  Serial.print("Client ID: ");
  Serial.println(client_id);
  Serial.print("Buffer size: ");
  Serial.println(client.getBufferSize());
  Serial.println("Callback MQTT configurado: receivedCallback");
  Serial.println("==========================");
  setTime();                    //Ajusta el tiempo del dispositivo con servidores SNTP
  setupSHT();             //Configura el sensor SHT21
}


/**
 * Configura el sensor SHT21
 */
void setupSHT() {
  if (sht.init()) Serial.print("SHT init(): Exitoso\n");
  else Serial.print("SHT init(): Fallido\n");
  sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM); // soportado solo por el SHT3x
}


/**
 * Configura el sensor MAX30100
 */
void setupMAX30100() {
  Serial.print("Inicializando MAX30100...");
  if (!pox.begin()) {
    Serial.println(" FALLO");
    for(;;); // Detiene el programa si falla
  } else {
    Serial.println(" OK");
  }
  // Configurar el sensor
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  // Registrar callback para cada latido detectado
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

/**
 * Configura el sensor MPU6050
 */
void setupMPU6050() {
  Serial.print("Inicializando MPU6050...");
  Wire.begin();
  mpu.initialize();
  
  if (!mpu.testConnection()) {
    Serial.println(" FALLO");
    for(;;); // Detiene el programa si falla
  } else {
    Serial.println(" OK");
  }
  
  // Configurar rangos
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2); // ±2g
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);  // ±250°/s
}

/**
 * Callback que se ejecuta cuando MAX30100 detecta un latido
 */
void onBeatDetected() {
  Serial.println("💓 Latido detectado!");
}

/**
 * Verifica si ya es momento de hacer las mediciones de las variables.
 * si ya es tiempo, mide y envía las mediciones.
 */
bool measure(SensorData * data) {
  if (millis() - measureTime > MEASURE_INTERVAL * 1000) {
    measureTime = millis();
    
    // Leer SHT21 (temperatura y humedad)
    if (sht.readSample()) {
      data->temperature = sht.getTemperature();
      data->humidity = sht.getHumidity();
      PRINT("Temperatura: ");
      PRINTD(data->temperature, 2);
      PRINT(" Humedad: ");
      PRINTD(data->humidity, 2);
      PRINTLN("");
    } else {
      PRINTLN("Error leyendo SHT21");
    }
    
    // Leer MAX30100 (frecuencia cardíaca y SpO2)
    pox.update();
    data->heartRate = pox.getHeartRate();
    data->spO2 = pox.getSpO2();
    PRINT("BPM: ");
    PRINTD(data->heartRate, 1);
    PRINT(" SpO2: ");
    PRINTD(data->spO2, 1);
    PRINTLN("%");
    
    // Leer MPU6050 (acelerómetro y giroscopio)
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    // Convertir a unidades reales
    data->accelX = ax / 16384.0; // Para rango ±2g
    data->accelY = ay / 16384.0;
    data->accelZ = az / 16384.0;
    data->gyroX = gx / 131.0;    // Para rango ±250°/s
    data->gyroY = gy / 131.0;
    data->gyroZ = gz / 131.0;
    
    PRINT("Accel X:");
    PRINTD(data->accelX, 2);
    PRINT(" Y:");
    PRINTD(data->accelY, 2);
    PRINT(" Z:");
    PRINTD(data->accelZ, 2);
    PRINTLN("");
    
    return true;
  }
  
  // Actualizar MAX30100 continuamente (necesario para detección de latidos)
  pox.update();
  return false;
}

/**
 * Verifica si ha llegdo alguna alerta al dispositivo.
 * Si no ha llegado devuelve OK, de lo contrario retorna la alerta.
 * También asigna el tiempo en el que se dispara la alerta.
 */
String checkAlert() {
  if (alert.length() != 0) {
    if ((millis() - alertTime) >= ALERT_DURATION * 1000 ) {
      alert = "";
      alertTime = millis();
    }
    return alert;
  } else return "OK";
}

/**
 * Publica la temperatura y humedad dadas al tópico configurado usando el cliente MQTT.
 */
void sendSensorData(SensorData* data) {
  // Crear JSON con todos los datos
  StaticJsonDocument<512> doc;
  
  doc["temperature"] = data->temperature;
  doc["humidity"] = data->humidity;
  doc["heartRate"] = data->heartRate;
  doc["spO2"] = data->spO2;
  doc["accelX"] = data->accelX;
  doc["accelY"] = data->accelY;
  doc["accelZ"] = data->accelZ;
  doc["gyroX"] = data->gyroX;
  doc["gyroY"] = data->gyroY;
  doc["gyroZ"] = data->gyroZ;
  doc["timestamp"] = now;
  doc["device_id"] = getMacAddress();
  
  char buffer[512];
  serializeJson(doc, buffer);
  
  PRINT("Publicando: ");
  PRINTLN(buffer);
  
  if (client.publish(MQTT_TOPIC_PUB, buffer)) {
    Serial.println("✓ Datos publicados correctamente");
  } else {
    Serial.println("✗ Error al publicar datos");
  }
}


/**
 * Función que se ejecuta cuando llega un mensaje a la suscripción MQTT.
 * Construye el mensaje que llegó y si contiene ALERT lo asgina a la variable 
 * alert que es la que se lee para mostrar los mensajes.
 * También verifica si el mensaje es para actualización OTA.
 */
void receivedCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("\n*** CALLBACK MQTT DISPARADO ***");
  Serial.print("Topic recibido: [");
  Serial.print(topic);
  Serial.print("] (longitud payload: ");
  Serial.print(length);
  Serial.println(")");
  
  // Crear buffer para el payload (agregar null terminator)
  // Usar String para evitar problemas con VLA
  String data = "";
  for (unsigned int i = 0; i < length; i++) {
    data += (char)payload[i];
  }
  
  Serial.print("Payload: ");
  Serial.println(data);
  
  // Compara el topic recibido con el topic OTA
  String topicStr = String(topic);
  String otaTopicStr = String(OTA_TOPIC);
  
  Serial.println("--- Comparación de topics ---");
  Serial.print("Topic recibido: '");
  Serial.print(topicStr);
  Serial.print("' (longitud: ");
  Serial.print(topicStr.length());
  Serial.println(")");
  Serial.print("OTA_TOPIC esperado: '");
  Serial.print(otaTopicStr);
  Serial.print("' (longitud: ");
  Serial.print(otaTopicStr.length());
  Serial.println(")");
  Serial.print("¿Coinciden? ");
  Serial.println(topicStr == otaTopicStr ? "SÍ" : "NO");
  
  // Verifica si el mensaje es para actualización OTA
  if (topicStr == otaTopicStr) {
    Serial.println("✓✓✓ Mensaje OTA detectado, procesando...");
    checkOTAUpdate(data.c_str());
    return;
  }
  
  // Verifica si el mensaje contiene una alerta
  if (data.indexOf("ALERT") >= 0) {
    Serial.println("✓ Mensaje ALERT detectado");
    alert = data; // Si el mensaje contiene la palabra ALERT, se asigna a la variable alert
  } else {
    Serial.println("⚠ Mensaje recibido pero no es OTA ni ALERT");
  }
  Serial.println("*** FIN CALLBACK ***\n");
}

/**
 * Función de prueba: Publica un mensaje de prueba y verifica recepción
 * Útil para diagnosticar problemas de MQTT
 */
void testMQTTCallback() {
  if (!client.connected()) {
    Serial.println("⚠ No se puede probar: cliente MQTT no conectado");
    return;
  }
  
  Serial.println("=== TEST MQTT CALLBACK ===");
  Serial.println("Este test verifica que el callback funciona correctamente");
  Serial.println("Publicando mensaje de prueba...");
  
  // Publicar un mensaje de prueba al topic de entrada (para que el dispositivo lo reciba)
  String testTopic = String(MQTT_TOPIC_SUB);
  String testMessage = "TEST_MESSAGE_FROM_SELF";
  
  bool pubResult = client.publish(testTopic.c_str(), testMessage.c_str());
  if (pubResult) {
    Serial.println("✓ Mensaje de prueba publicado");
    Serial.println("Esperando recibirlo en el callback...");
    Serial.println("(Si el callback funciona, deberías ver '*** CALLBACK MQTT DISPARADO ***' arriba)");
  } else {
    Serial.println("✗ Error al publicar mensaje de prueba");
  }
  
  // Procesar mensajes varias veces para asegurar recepción
  for (int i = 0; i < 10; i++) {
    client.loop();
    delay(100);
  }
  
  Serial.println("=== FIN TEST ===");
}