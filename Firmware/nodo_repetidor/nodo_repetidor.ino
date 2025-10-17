//
// Nodo Repetidor de ESP-NOW
//
// Este sistema recibe un mensaje de ESP-NOW y lo reenvia a otro destino
// Autor: Juan Tiago Ruiz Rodriguez

// Importa las librerias necesarias
#include <esp_now.h>
#include <WiFi.h>

// ============================================================================
// CONFIGURACIÓN DE DESTINO
// ============================================================================

// IMPORTANTE: Reemplaza esta dirección MAC con la del ESP32 que recibirá el mensaje reenviado.
// Puedes usar 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF para enviar a todos (broadcast),
// pero es más eficiente y seguro usar una dirección específica.
uint8_t direccionDestino[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// ============================================================================
// ESTRUCTURA DE DATOS
// ============================================================================

// Se mantiene la misma estructura de datos del mensaje original
typedef struct struct_message {
  int id;
  int b;
  int c;
  int type;
  int readingID;
  int traffic_density[4]; // Density values or unused
} struct_message;

// Crea un struct_message para almacenar los datos temporalmente
struct_message myData;

// Variable para la información del par (peer)
esp_now_peer_info_t peerInfo;

// ============================================================================
// FUNCIONES CALLBACK DE ESP-NOW
// ============================================================================

// Función que se ejecuta cuando se reciben datos
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
  // Copia los datos recibidos a la estructura myData
  memcpy(&myData, incomingData, sizeof(myData));

  // Imprime en el monitor serial que se recibió un mensaje
  Serial.print("Paquete recibido desde: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info->src_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  Serial.print("Bytes: ");
  Serial.println(len);

  // Reenvía el mensaje a la dirección de destino
  esp_err_t result = esp_now_send(direccionDestino, (uint8_t *) &myData, sizeof(myData));

  // Confirma si el reenvío fue exitoso o falló
  if (result == ESP_OK) {
    Serial.println("Mensaje reenviado con exito.");
  } else {
    Serial.println("Error al reenviar el mensaje.");
  }
  Serial.println("-------------------------------------");
}

// Función que se ejecuta cuando se ha enviado un dato (para saber el status)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Estado del ultimo paquete reenviado: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Entrega exitosa" : "Fallo en la entrega");
}

// ============================================================================
// FUNCION SETUP
// ============================================================================

void setup() {
  // Inicia la comunicación serial para depuración
  Serial.begin(115200);

  // Configura el dispositivo en modo Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Inicia ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  // Registra la función callback para cuando se envíen datos
  esp_now_register_send_cb(OnDataSent);

  // Registra la función callback para cuando se reciban datos
  esp_now_register_recv_cb(OnDataRecv);

  // Registra el par (peer) al que se le reenviarán los datos
  memcpy(peerInfo.peer_addr, direccionDestino, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Añade el par a la lista de ESP-NOW
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Fallo al anadir el par de destino");
    return;
  }
  
  Serial.println("Nodo Repetidor ESP-NOW listo.");
  Serial.println("Esperando mensajes para reenviar...");
}

// ============================================================================
// FUNCION LOOP
// ============================================================================

void loop() {
  // No se necesita nada aquí, ya que el proceso es asíncrono
  // y se maneja completamente con las funciones callback.
}