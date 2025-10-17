//
// Unidad Detectora de Luz de Trafico 
//
// Este sistema envia señal de presencia de vehiculos al Nodo de Control
// Autor: Juan Tiago Ruiz Rodriguez

#include "esp_camera.h"
#include <esp_wifi.h>
#include <WiFi.h>
#include <esp_now.h>

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_ESP_EYE  // Has PSRAM
#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#define USE_WEB_SERVER
//#define DEBUG // usado para que imprima los datos por serial
#define USE_ESP_NOW
// definición para que encienda led en verde si detecta, en rojo si no
#define GREEN_ON  rgbLedWrite(RGB_BUILTIN, 0, 255, 0)
#define RED_ON    rgbLedWrite(RGB_BUILTIN, 255, 0, 0)
#define LED_OFF   rgbLedWrite(RGB_BUILTIN, 0, 0, 0)

#include "camera_pins.h"

#define LED_GPIO_PIN          2
#define BOARD_ID              2 // definir ID de acuerdo a cada unidad detectora

// ===========================
// Configurar credenciales wifi
// ===========================
const char *ssid = "DTO703";
const char *password = "pistachos";

bool bOn = false;

extern int ei_edge_impulse(camera_fb_t *fb);
extern void startCameraServer();
extern void setupLedFlash(int pin);

#if defined(USE_ESP_NOW)
//uint8_t broadcastAddress[] = {0xFC, 0x01, 0x2C, 0xD2, 0x03, 0xF8}; // MAC del ESP32 receptor
uint8_t broadcastAddress[] = {0x08, 0x3A, 0xF2, 0xAA, 0x06, 0x80}; // MAC del ESP32S receptor

int zeroVehicleCount = 0;
bool flagSent = false;

// Estructura a enviar por ESPNOW
typedef struct struct_message {
  int id;
  int b;
  int c;
  int readingID;
  int type;
  int traffic_density[4]; // Density values or unused
} struct_message;
// Crea un struct_message llamado myData
struct_message myData;
unsigned int readingId = 0;

esp_now_peer_info_t peerInfo;

// callback cuando se envía información
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  #if defined(DEBUG)
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Entrega exitosa" : "Falla en la entrega");
  #endif
  // APAGAR FLAG DE AVISO DE ESTADO si el mensaje se entrego satisfactoriamente
  if (status == ESP_NOW_SEND_SUCCESS && flagSent) {
    flagSent = false;
    zeroVehicleCount = 0;
    #if defined(DEBUG)
    Serial.println("Flag reseteada - mensaje entregado satisfactoriamente");
    #endif
  }
}
#endif


#if defined(USE_WEB_SERVER)
int32_t getWiFiChannel(const char *ssid) { // se utiliza para que envie ESP-NOW por el mismo canal que wifi del ESP32 central, no se conecta
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}
#endif

void setup() {

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  pinMode(LED_GPIO_PIN, OUTPUT);
  // configuración de cámara
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Inicializacion de camara fallo con error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 0);
#endif

// Setup LED FLash if LED pin is defined in camera_pins.h
#if defined(LED_GPIO_NUM)
  setupLedFlash(LED_GPIO_NUM);
#endif

 // Set device as a Wi-Fi Station
WiFi.mode(WIFI_STA);

#if defined(USE_WEB_SERVER)
  
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    bOn = !bOn;
    delay(250);
    Serial.print(".");
    digitalWrite(LED_GPIO_PIN, bOn ? LOW : HIGH);
  }
  Serial.println("");
  Serial.println("WiFi connectado");

  startCameraServer();

  Serial.print("Camara lista! Usar 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' para conectarse");
#endif

#if defined(USE_ESP_NOW)



  #if defined(USE_WEB_SERVER)
  int32_t channel = getWiFiChannel(ssid);
  // Ajusta el canal de wifi al mismo que esp now para poder usar ambas cosas
  //WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  //WiFi.printDiag(Serial); // Uncomment to verify channel change after
  #endif

  // Inicializa ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Una vez inicializado, se registra el callback para que nos informe el estado del paquete transmitido
  esp_now_register_send_cb(OnDataSent);

  // Registra un par
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Agrega par       
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Fallo al añadir par");
    return;
  }

#endif

  Serial.printf("HeapSize: %d bytes\n", ESP.getHeapSize());
  Serial.printf("FreeHeap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("MinFreeHeap: %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("MaxAllocHeap: %d bytes\n", ESP.getMaxAllocHeap());
  
  // PSRAM
  if (ESP.getPsramSize() > 0) {
    Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());
    Serial.printf("FreePSRAM: %d bytes\n", ESP.getFreePsram());
  }
  
  Serial.printf("SketchSize: %d bytes\n", ESP.getSketchSize());
  Serial.printf("FreeSketchSpace: %d bytes\n", ESP.getFreeSketchSpace());
  #if defined(USE_ESP_NOW)
  myData.type = 0; // tipo de dato: detección
  myData.traffic_density[1] = 0; // Valores de densidad sin utilizar
  myData.traffic_density[2] = 0;
  myData.traffic_density[3] = 0;
  myData.traffic_density[4] = 0;
  #endif
}

void loop() {

  camera_fb_t *fb = esp_camera_fb_get();

  if( fb )
  {
    int numVehicles = ei_edge_impulse(fb); // obtiene la cuenta
    esp_camera_fb_return(fb);
    if (numVehicles > 0) {
      #if defined(DEBUG)
      GREEN_ON; // enciende led verde
      #endif
      #if defined(USE_ESP_NOW)
      // Resetea el contador cuando se detecta vehiculo
      zeroVehicleCount = 0;
      flagSent = false;
      #endif
    }
    else {
      #if defined(DEBUG)
      RED_ON; // enciende led rojo
      #endif
      #if defined(USE_ESP_NOW)
      // Incrementa contador cuando no hay vehiculos detectados
      zeroVehicleCount++;
      #endif
    } 
    Serial.printf("Vehiculos detectados: %d\n", numVehicles);
    #if defined(USE_ESP_NOW)
    // Define valores a enviar
    myData.id = BOARD_ID;
    myData.b = numVehicles;
    myData.readingID = readingId++;

    // Si se producen 4 detecciones seguidas sin vehiculos, activa el flag
    if (zeroVehicleCount >= 4 && !flagSent) {
      myData.c = 1;  // Coloca el flag en 1
      flagSent = true; // activa flag hasta que se confirma la recepcion
      Serial.println("Flag en 1 - no se detectaron vehículos por 4 ciclos");
    } else {
      myData.c = 0;  // Operacion normal
    }

    // Envia mensaje por ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    #if defined(DEBUG)     
    if (result == ESP_OK) {

      Serial.println("Enviado con exito");
    }
    else {
      Serial.println("Error al enviar mensaje");
    }
    #endif
    #endif
  }
  else
  {
    Serial.println("Falla en la captura de camara !!!!!! Esperar...");
    digitalWrite(LED_GPIO_PIN, bOn ? LOW : HIGH);
    bOn = !bOn;
  
    delay(500);
  }
}
