//
// Nodo de Control de Luz de Trafico 
//
// Este sistema controla una intersección de vías con datos que recibe de las Unidades Detectoras
// Autor: Juan Tiago Ruiz Rodriguez


// Importa las librerias necesarias
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <esp_now.h>

// ============================================================================
// DEFINICIONES DE PINS
// ============================================================================

// Leds de luces de trafico (o salidas de relé para controlador de tráfico)
// Direccion Norte (Direccion 0)
const unsigned char northRed = 23;
const unsigned char northYellow = 22;
const unsigned char northGreen = 21;

// Direccion Este (Direccion 1)
const unsigned char eastRed = 19;
const unsigned char eastYellow = 18;
const unsigned char eastGreen = 5;

// Entradas de sensores (para pruebas)
//const unsigned char sensor1 = 7;  // North sensor
//const unsigned char sensor2 = 8;  // East sensor


// Boton de emergencia/mantenimiento
const unsigned char manualOverride = 15;

// ============================================================================
// CONSTANTES DE TIEMPO 
// ============================================================================

// Tiempo minimo, maximo, y de amarillo para cada fase (milisegundos)
const unsigned long tiempo_min = 5000;   // 5 segundos minimo
const unsigned long tiempo_max = 20000;  // 20 segundos maximo
const unsigned long tiempo_amarillo = 3000; // 3 segundos amarillo
const unsigned long tiempo_semaforo = 2000; // 2 segundos todo rojo emergencia

// Variables de tiempo
unsigned long timerMillis;
#define accumulatedMillis millis() - timerMillis

// ============================================================================
// DEFINICIONES DE ESTADOS
// ============================================================================

// Estados de luz de trafico basados en el diagrama de maquina de estados
enum TrafficStates {
  INICIO,     // 0 - Estado Inicial - Todo Rojo
  GR,       // 1 - Norte Verde, otros Rojo
  YR,       // 2 - Norte Amarillo, otros Rojo
  RG,       // 3 - Este Verde, otros Rojo
  RY,       // 4 - Este Amarillo, otros Rojo
  YYYY        // 9 - Todos Amarillo (seguridad/mantenimiento)
};

unsigned char trafficState;

// ============================================================================
// VARIABLES DE TIMEOUT DE SENSOR
// ============================================================================

const unsigned long SENSOR_TIMEOUT = 10000;  // 10 segundos de timeout (tiempo sin recibir señal del detector)
unsigned long lastSensorReceived[4] = {millis(), millis(), millis(), millis()};  // Almacena los tiempos de recepcion de cada sensor
const unsigned long tiempo_default = 15000;  // Tiempo verde default (para timeout)
bool sensorTimedOut[4] = {false, false, false, false};

// ============================================================================
// VARIABLES DE CONTROL Y SENSORES
// ============================================================================

bool sensorActive[4] = {false, false};
float traffic_density [4] = {0.0, 0.0, 0.0, 0.0};
unsigned long phaseStartTime;
bool emergencyMode = false;
unsigned char previousState = 255;  // Inicializa a un estado no valido

// Configurar credenciales wifi
const char* ssid = "DTO703";
const char* password = "pistachos";

// Crea un objeto AsyncWebServer en el puerto 80
AsyncWebServer server(80);

// Valores de intensidad de trafico de sensores
int sensor_2_intensity = 70;  // Heroes de Malvinas y Sarmiento
int sensor_3_intensity = 70;  // Heroes de Malvinas y San Juan
int sensor_4_intensity = 70;  // Junin y Av. Peron
int sensor_5_intensity = 70;  // Republica del Libano y Sarmiento
int sensor_6_intensity = 70;  // Heroes de Malvinas y Av. Peron EO
int sensor_7_intensity = 70;  // Peron y Heroes de Malvinas NS
int sensor_8_intensity = 70;  // Peron y Heroes de Malvinas SN

// Estructura a enviar por ESPNOW
typedef struct struct_message {
  int id;
  int b;
  int c;
  int type;
  int readingID;
  int traffic_density[4]; // Density values or unused
} struct_message;
// Crea un struct_message llamado myData
struct_message myData;

// al recibir msj de esp now imprime datos y actualiza
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes recibidos: ");
  Serial.println(len);
  Serial.print("Desde MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info->src_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  Serial.print("Board ID: ");
  Serial.println(myData.id);
  Serial.print("Cantidad de vehiculos: ");
  Serial.println(myData.b);
  Serial.print("Flag de deteccion de brecha: ");
  Serial.println(myData.c);
  Serial.print("# de medicion: ");
  Serial.println(myData.readingID);
  Serial.println();

  sensorActive[myData.id-1] = myData.c;
  lastSensorReceived[myData.id-1] = millis();  // Guarda el momento de recepcion
  sensorTimedOut[myData.id-1] = false;  // Limpia el flag de timeout
}

// ============================================================================
// FUNCION SETUP
// ============================================================================

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Inicializa LittleFS
  if(!LittleFS.begin()){
    Serial.println("Un error ocurrio al montar LittleFS");
    return;
  }
  WiFi.mode(WIFI_AP_STA);
  // Conecta a Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi..");
  }
  Serial.print("Direccion IP de la estacion: ");
  Serial.println(WiFi.localIP());
  Serial.print("Canal Wi-Fi: ");
  Serial.println(WiFi.channel());

  if (esp_now_init() != ESP_OK) {
  Serial.println("Error inicializando ESP-NOW");
  return;
  } 

  // accion al recibir dato esp-now
  esp_now_register_recv_cb(OnDataRecv);

  // Direccion para root / pagina web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index2.html", String(), false);
  });
  // Direccion para cargar archivo style.css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });
  // Direccion para cargar archivo javascript
  server.on("/heatmap.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
   request->send(LittleFS, "/heatmap.min.js", "application/javascript");
  });
  // Direccion para cargar imagenes
  server.on("/mapa_trafico2.PNG", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/mapa_trafico2.PNG", "image/png");
  });

  server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        
        StaticJsonDocument<1024> jsonDoc;
        
        jsonDoc["sensor_2"] = sensor_2_intensity;
        jsonDoc["sensor_3"] = sensor_3_intensity;
        jsonDoc["sensor_4"] = sensor_4_intensity;
        jsonDoc["sensor_5"] = sensor_5_intensity;
        jsonDoc["sensor_6"] = sensor_6_intensity;
        jsonDoc["sensor_7"] = sensor_7_intensity;
        jsonDoc["sensor_8"] = sensor_8_intensity;
        
        serializeJson(jsonDoc, *response);
        request->send(response);
    });

  // Inicia el servidor
  server.begin();
  randomSeed(analogRead(0));


  Serial.println("Controlador de Luz de Trafico Iniciando...");
  // Inicializa los LED de salida
  pinMode(northRed, OUTPUT);
  pinMode(northYellow, OUTPUT);
  pinMode(northGreen, OUTPUT);
  pinMode(eastRed, OUTPUT);
  pinMode(eastYellow, OUTPUT);
  pinMode(eastGreen, OUTPUT);
  

  // Inicializa entradas de sensores (pruebas) y emergencia
  //pinMode(sensor1, INPUT);
  //pinMode(sensor2, INPUT);
  pinMode(manualOverride, INPUT_PULLUP);

  // Comienza en estado inicial y almacena instante
  trafficState = INICIO;
  timerMillis = millis();

  // Coloca todas las luces en rojo inicialmente
  setAllRed();

  Serial.println("Sistema inicializado - Todo Rojo");
  Serial.println("Secuencia completa:INICIO   →GR→YR→RG→RY→repite");

}

 void loop() {
  // Genera intensidades aleatorias para los sensores que no serian de la interseccion 1–100
  sensor_2_intensity = random(1, 101);
  sensor_3_intensity = random(1, 101);
  sensor_4_intensity = random(1, 101);
  sensor_5_intensity = random(1, 101);
  sensor_6_intensity = traffic_density[0];
  sensor_7_intensity = traffic_density[1];
  sensor_8_intensity = 0;

  // Al actualizar, se entregan valores nuevos

  // Revisa si los detectores envian info
  checkSensorTimeouts();

  // Checkea por override de emergencia
  if (digitalRead(manualOverride) == LOW) {
    emergencyMode = true;
    trafficState = YYYY;
    setAllRed();
    Serial.println("MODO EMERGENCIA ACTIVADO");
    return;
  } else {
    emergencyMode = false;
  }

  // Maquina de estados principal
  switch (trafficState) {

    case INICIO:
      if (previousState != INICIO) {
        Serial.println("Estado: INICIO (Inicializacion - Todo Rojo)");
        previousState = INICIO;  // Actualiza el estado previo para imprimir una sola vez
      } 
      setAllRed();
      if (accumulatedMillis >= tiempo_semaforo) { // mientras el tiempo no haya pasado, se mantiene
        timerMillis = millis();
        trafficState = GR;  // Comienza con direccion norte
      }
      break;

    case GR:
      if (previousState != GR) {
        Serial.println("Estado: GR (Norte Verde, otros Rojo)");
        previousState = GR;  // Actualiza el estado previo para imprimir una sola vez
      }
      setTrafficLights(1, 0, 0, 0); // Coloca las luces en Norte Verde, otros Rojo

      // Revisa las condiciones de transicion
      if (accumulatedMillis >= tiempo_min) {
        // Tiempo minimo cumplido, revisa por razones para cambiar
        if (accumulatedMillis >= tiempo_max || 
            (sensorActive[0]) || (sensorTimedOut[0] && accumulatedMillis >= tiempo_default)) {

        // Captura la duracion del verde para obtener estadistica de densidad
          unsigned long greenLightDuration = accumulatedMillis;

          // Convierte a medida de densidad de trafico (1-100%)
          traffic_density[0] = ((float)(greenLightDuration - tiempo_min) / (float)(tiempo_max - tiempo_min)) * 100.0;
          
          // Ajusta al rango 1-100
          if (traffic_density[0] < 1.0) traffic_density[0] = 1.0;
          if (traffic_density[0] > 100.0) traffic_density[0] = 100.0;

          Serial.println("========================================");
          Serial.print("DURACION DE LUZ VERDE NORTE: ");
          Serial.print(greenLightDuration);
          Serial.print(" ms (");
          Serial.print(greenLightDuration / 1000.0, 1);
          Serial.println(" segundos)");

          Serial.print("DENSIDAD DE TRAFICO: ");
          Serial.print(traffic_density[0], 1);
          Serial.println("%");
          
          // Imprime por que cambio la luz verde
          if (accumulatedMillis >= tiempo_max) {
            Serial.printf("Razon: Tiempo maximo limite (%lus) alcanzado\n", tiempo_max/1000);
          } else if (sensorActive[0]) {
            Serial.println("Razon: Brecha detectada - transicion temprana");
          } else if (sensorTimedOut[0]) {
            Serial.println("Razon: Sensor no detectado - tiempo verde predeterminado");
          }
          Serial.println("========================================");
          timerMillis = millis();
          trafficState = YR;
        }
      }
      break;

    case YR:
      if (previousState != YR) {
        Serial.println("Estado: YR (Norte Amarillo, otros Rojo)");
        previousState = YR;  // Actualiza el estado previo para imprimir una sola vez
      }
      
      setTrafficLights(2, 0, 0, 0); // Coloca las luces en Norte Amarillo, otros Rojo

      if (accumulatedMillis >= tiempo_amarillo) { // espera a que pase el tiempo de amarillo
        timerMillis = millis();
        trafficState = RG;
      }
      break;

    case RG:
      if (previousState != RG) {
        Serial.println("Estado: RG (Este Verde, otros Rojo)");
        previousState = RG;  // Actualiza el estado previo para imprimir una sola vez
      }
      
      setTrafficLights(0, 1, 0, 0); // Este Verde, otros Rojo

      if (accumulatedMillis >= tiempo_min) {
        if (accumulatedMillis >= tiempo_max || 
            (sensorActive[1]) || (sensorTimedOut[1] && accumulatedMillis >= tiempo_default)) {

          unsigned long greenLightDuration = accumulatedMillis;

          traffic_density[1] = ((float)(greenLightDuration - tiempo_min) / (float)(tiempo_max - tiempo_min)) * 100.0;
          
          if (traffic_density[1] < 1.0) traffic_density[1] = 1.0;
          if (traffic_density[1] > 100.0) traffic_density[1] = 100.0;

          Serial.println("========================================");
          Serial.print("DURACION DE LUZ VERDE ESTE: ");
          Serial.print(greenLightDuration);
          Serial.print(" ms (");
          Serial.print(greenLightDuration / 1000.0, 1);
          Serial.println(" segundos)");
          
          Serial.print("DENSIDAD DE TRAFICO: ");
          Serial.print(traffic_density[1], 1);
          Serial.println("%");

          // Print why the light changed
          if (accumulatedMillis >= tiempo_max) { // igual a estado anterior, cambia los valores de sensoractive y sensortimedout
            Serial.printf("Razon: Tiempo maximo limite (%lus) alcanzado\n", tiempo_max/1000);
          } else if (sensorActive[1]) {
            Serial.println("Razon: Brecha detectada - transicion temprana");
          } else if (sensorTimedOut[1]) {
            Serial.println("Razon: Sensor no detectado - tiempo verde predeterminado");
          }
          Serial.println("========================================");

          timerMillis = millis();
          trafficState = RY;
        }
      }
      break;

    case RY:
      if (previousState != RY) {
        Serial.println("Estado: RY (Este Amarillo, otros Rojo)");
        previousState = RY;  // Actualiza el estado previo para imprimir una sola vez
      }
      
      setTrafficLights(0, 2, 0, 0); // Este Amarillo, otros Rojo

      if (accumulatedMillis >= tiempo_amarillo) {
        timerMillis = millis();
        trafficState = GR; // Vuelve a direccion norte
      }
      break;


    case YYYY:
      Serial.println("ESTADO: RRRR (Todo Rojo Emergencia)");
      setAllRed();

      if (accumulatedMillis >= tiempo_semaforo) {
        timerMillis = millis();
        trafficState = GR;
      }
      break;

    default:
      Serial.println("ERROR: Estado desconocido - volviendo a INICIO");
      trafficState = INICIO;
      timerMillis = millis();
      break;
  }

  delay(100);
}

// ============================================================================
// FUNCIONES DE AYUDA
// ============================================================================
void checkSensorTimeouts() {
  unsigned long currentTime = millis();
  
  for (int i = 0; i < 4; i++) {
    // Revisa si el sensor esta en timeout (han pasado 10 segundos de la ultima deteccion)
    if (lastSensorReceived[i] > 0 && (currentTime - lastSensorReceived[i]) >= SENSOR_TIMEOUT) {
      if (!sensorTimedOut[i]) {
        sensorTimedOut[i] = true;
        Serial.print("ADVERTENCIA: Sensor ");
        Serial.print(i);
        Serial.println(" time out - utilizando tiempos predeterminados");
      }
    }
  }
}

void setAllRed() {
  // Coloca todas las luces en rojo, apaga verdes y amarillos
  digitalWrite(northRed, HIGH);
  digitalWrite(northYellow, LOW);
  digitalWrite(northGreen, LOW);

  digitalWrite(eastRed, HIGH);
  digitalWrite(eastYellow, LOW);
  digitalWrite(eastGreen, LOW);

}

void setTrafficLights(int north, int east, int south, int west) {
  // Coloca todas las luces para cada direccion
  // 0 = Rojo, 1 = Verde, 2 = Amarillo

  setDirectionLight(north, northRed, northYellow, northGreen);
  setDirectionLight(east, eastRed, eastYellow, eastGreen);
}

void setDirectionLight(int state, int redPin, int yellowPin, int greenPin) {
  switch(state) {
    case 0: // Rojo
      digitalWrite(redPin, HIGH);
      digitalWrite(yellowPin, LOW);
      digitalWrite(greenPin, LOW);
      break;
    case 1: // Verde
      digitalWrite(redPin, LOW);
      digitalWrite(yellowPin, LOW);
      digitalWrite(greenPin, HIGH);
      break;
    case 2: // Amarillo
      digitalWrite(redPin, LOW);
      digitalWrite(yellowPin, HIGH);
      digitalWrite(greenPin, LOW);
      break;
    default: // Emergencia - Rojo
      digitalWrite(redPin, HIGH);
      digitalWrite(yellowPin, LOW);
      digitalWrite(greenPin, LOW);
      break;
  }
}