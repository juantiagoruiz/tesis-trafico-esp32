//
// Nodo de Control de Luz de Trafico 
//
// Este sistema controla una intersección de vías con datos que recibe de las Unidades Detectoras (sensores)
// No posee timeout de sensores
// Autor: Juan Tiago Ruiz Rodriguez

// ============================================================================
// DEFINICIONES DE PINS
// ============================================================================

// Leds de luces de trafico (o salidas de relé para controlador de tráfico)
// Direccion Norte (Direccion 0)
const unsigned char northRed = 2;
const unsigned char northYellow = 42;
const unsigned char northGreen = 41;

// Direccion Este (Direccion 1)
const unsigned char eastRed = 5;
const unsigned char eastYellow = 6;
const unsigned char eastGreen = 7;

// Direccion Sur (Direccion 2)
const unsigned char southRed = 9;
const unsigned char southYellow = 10;
const unsigned char southGreen = 11;

// Direccion Oeste (Direccion 4)
const unsigned char westRed = 12;
const unsigned char westYellow = 13;
const unsigned char westGreen = 14;

// Entradas de sensores (para pruebas)
const unsigned char sensor1 = 47;  // Norte sensor
const unsigned char sensor2 = 21;  // Este sensor
const unsigned char sensor3 = 20;  // Sur sensor
const unsigned char sensor4 = 19;  // Oeste sensor

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
  GRRR,       // 1 - Norte Verde, otros Rojo 
  YRRR,       // 2 - North Yellow, otros Rojo
  RGRR,       // 3 - Este Verde, otros Rojo
  RYRR,       // 4 - Este Amarillo, otros Rojo
  RRGR,       // 5 - Sur Verde, otros Rojo
  RRYR,       // 6 - Sur Amarillo, otros Rojo
  RRRG,       // 7 - Oeste Verde, otros Rojo
  RRRY,       // 8 - Oeste Amarillo, otros Rojo
  YYYY        // 9 - Todos Amarillo (seguridad/mantenimiento)
};

unsigned char trafficState;

// ============================================================================
// VARIABLES DE CONTROL Y SENSORES
// ============================================================================

bool sensorActive[4] = {false, false, false, false};
float traffic_density [4] = {0.0, 0.0, 0.0, 0.0};
unsigned long phaseStartTime;
bool emergencyMode = false;
unsigned char previousState = 255;  // Inicializa a un estado no valido
// ============================================================================
// FUNCION SETUP 
// ============================================================================

void setup() {
  Serial.begin(115200);
  Serial.println("Controlador de Luz de Trafico Iniciando...");
  // Inicializa los LED de salida
  pinMode(northRed, OUTPUT);
  pinMode(northYellow, OUTPUT);
  pinMode(northGreen, OUTPUT);
  pinMode(eastRed, OUTPUT);
  pinMode(eastYellow, OUTPUT);
  pinMode(eastGreen, OUTPUT);
  pinMode(southRed, OUTPUT);
  pinMode(southYellow, OUTPUT);
  pinMode(southGreen, OUTPUT);
  pinMode(westRed, OUTPUT);
  pinMode(westYellow, OUTPUT);
  pinMode(westGreen, OUTPUT);

  // Inicializa entradas de sensores (pruebas) y emergencia
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  pinMode(sensor3, INPUT);
  pinMode(sensor4, INPUT);
  pinMode(manualOverride, INPUT_PULLUP);

  // Comienza en estado inicial
  trafficState = INICIO;
  timerMillis = millis();

  // Coloca todas las luces en rojo inicialmente
  setAllRed();

  Serial.println("Sistema inicializado - Todo Rojo");
  Serial.println("Secuencia completa: INICIO→GRRR→YRRR→RGRR→RYRR→RRGR→RRYR→RRRG→RRRY→repite");
}


void loop() {
  // Read sensors
  readSensors();

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
      Serial.println("Estado: INICIO (Inicializacion - Todo Rojo)");
      setAllRed();
      if (accumulatedMillis >= tiempo_semaforo) {
        timerMillis = millis();
        trafficState = GRRR;  // Comienza con direccion norte
      }
      break;

    case GRRR:
      if (previousState != GRRR) {
        Serial.println("Estado: GRRR (Norte Verde, otros Rojo)");
        previousState = GRRR;  // Actualiza el estado previo para imprimir una sola vez
      }
      setTrafficLights(1, 0, 0, 0); // Coloca las luces en Norte Verde, otros Rojo

      // Revisa las condiciones de transicion
      if (accumulatedMillis >= tiempo_min) {
        // Tiempo minimo cumplido, revisa por razones para cambiar
        if (accumulatedMillis >= tiempo_max || 
            (!sensorActive[0])) {

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
            Serial.println("Razon: Tiempo maximo limite alcanzado\n");
          } else if (!sensorActive[0]) {
            Serial.println("Razon: Brecha detectada - transicion temprana");
          }
          Serial.println("========================================");
          timerMillis = millis();
          trafficState = YRRR;
        }
      }
      break;

    case YRRR:
      if (previousState != YRRR) {
        Serial.println("Estado: YRRR (Norte Amarillo, otros Rojo)");
        previousState = YRRR;
      }
      
      setTrafficLights(2, 0, 0, 0);

      if (accumulatedMillis >= tiempo_amarillo) {
        timerMillis = millis();
        trafficState = RGRR;
      }
      break;

    case RGRR:
      if (previousState != RGRR) {
        Serial.println("Estado: RGRR (Este Verde, otros Rojo)");
        previousState = RGRR;
      }
      
      setTrafficLights(0, 1, 0, 0);

      if (accumulatedMillis >= tiempo_min) {
        if (accumulatedMillis >= tiempo_max || 
            (!sensorActive[1])) {

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

          // Imprime por que cambio la luz verde
          if (accumulatedMillis >= tiempo_max) {
            Serial.println("Razon: Tiempo maximo limite alcanzado\n");
          } else if (!sensorActive[1]) {
            Serial.println("Razon: Brecha detectada - transicion temprana");
          }
          Serial.println("========================================");

          timerMillis = millis();
          trafficState = RYRR;
        }
      }
      break;

    case RYRR:
      if (previousState != RYRR) {
        Serial.println("Estado: RYRR (Este Amarillo, otros Rojo)");
        previousState = RYRR;
      }
      
      setTrafficLights(0, 2, 0, 0);

      if (accumulatedMillis >= tiempo_amarillo) {
        timerMillis = millis();
        trafficState = RRGR;
      }
      break;

    case RRGR:
      if (previousState != RRGR) {
        Serial.println("Estado: RRGG (Sur Verde, otros Rojo)");
        previousState = RRGR;  
      }
      
      setTrafficLights(0, 0, 1, 0);

      if (accumulatedMillis >= tiempo_min) {
        if (accumulatedMillis >= tiempo_max || 
            (!sensorActive[2])) {

          unsigned long greenLightDuration = accumulatedMillis;

          traffic_density[2] = ((float)(greenLightDuration - tiempo_min) / (float)(tiempo_max - tiempo_min)) * 100.0;
          
          if (traffic_density[2] < 1.0) traffic_density[2] = 1.0;
          if (traffic_density[2] > 100.0) traffic_density[2] = 100.0;


          Serial.println("========================================");
          Serial.print("DURACION DE LUZ VERDE SUR: ");
          Serial.print(greenLightDuration);
          Serial.print(" ms (");
          Serial.print(greenLightDuration / 1000.0, 1);
          Serial.println(" segundos)");
          
          Serial.print("DENSIDAD DE TRAFICO: ");
          Serial.print(traffic_density[2], 1);
          Serial.println("%");

          // Imprime por que cambio la luz verde
          if (accumulatedMillis >= tiempo_max) {
            Serial.println("Razon: Tiempo maximo limite alcanzado\n");
          } else if (!sensorActive[2]) {
            Serial.println("Razon: Brecha detectada - transicion temprana");
          }
          Serial.println("========================================");

          timerMillis = millis();
          trafficState = RRYR;
        }
      }
      break;

    case RRYR:
      if (previousState != RRYR) {
        Serial.println("Estado: RRYY (Sur Amarillo, otros Rojo)");
        previousState = RRYR;
      }
      
      setTrafficLights(0, 0, 2, 0);

      if (accumulatedMillis >= tiempo_amarillo) {
        timerMillis = millis();
        trafficState = RRRG;
      }
      break;

    case RRRG:
      if (previousState != RRRG) {
        Serial.println("Estado: RRRG (Oeste Verde, otros Rojo)");
        previousState = RRRG;
      }
      
      setTrafficLights(0, 0, 0, 1);

      if (accumulatedMillis >= tiempo_min) {
        if (accumulatedMillis >= tiempo_max || 
            (!sensorActive[3])) {

          unsigned long greenLightDuration = accumulatedMillis;
          
          traffic_density[3] = ((float)(greenLightDuration - tiempo_min) / (float)(tiempo_max - tiempo_min)) * 100.0;
          
          if (traffic_density[3] < 1.0) traffic_density[3] = 1.0;
          if (traffic_density[3] > 100.0) traffic_density[3] = 100.0;

          Serial.println("========================================");
          Serial.print("DURACION DE LUZ VERDE OESTE: ");
          Serial.print(greenLightDuration);
          Serial.print(" ms (");
          Serial.print(greenLightDuration / 1000.0, 1);
          Serial.println(" seconds)");
          
          Serial.print("DENSIDAD DE TRAFICO: ");
          Serial.print(traffic_density[3], 1);
          Serial.println("%");

          // Imprime por que cambio la luz verde
          if (accumulatedMillis >= tiempo_max) {
            Serial.println("Razon: Tiempo maximo limite alcanzado\n");
          } else if (!sensorActive[3]) {
            Serial.println("Razon: Brecha detectada - transicion temprana");
          }
          Serial.println("========================================");

          timerMillis = millis();
          trafficState = RRRY;  
        }
      }
      break;

    case RRRY:
      if (previousState != RRRY) {
        Serial.println("Estado: RRRY (Oeste Amarillo, otros Rojo)");
        previousState = RRRY;
      }
      
      setTrafficLights(0, 0, 0, 2);

      if (accumulatedMillis >= tiempo_amarillo) {
        timerMillis = millis();
        trafficState = GRRR; // Repite
      }
      break;

    case YYYY:
      Serial.println("Estado: RRRR (Todos Rojos Emergencia)");
      setAllRed();

      if (accumulatedMillis >= tiempo_semaforo) {
        timerMillis = millis();
        trafficState = GRRR;
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

void readSensors() {
  // lee sensores digitales (simulacion de unidad detectora)
  sensorActive[0] = digitalRead(sensor1); // North
  sensorActive[1] = digitalRead(sensor2); // East  
  sensorActive[2] = digitalRead(sensor3); // South
  sensorActive[3] = digitalRead(sensor4); // West

}

void setAllRed() {
  // Coloca todas las luces en rojo, apaga verdes y amarillos
  digitalWrite(northRed, HIGH);
  digitalWrite(northYellow, LOW);
  digitalWrite(northGreen, LOW);

  digitalWrite(eastRed, HIGH);
  digitalWrite(eastYellow, LOW);
  digitalWrite(eastGreen, LOW);

  digitalWrite(southRed, HIGH);
  digitalWrite(southYellow, LOW);
  digitalWrite(southGreen, LOW);

  digitalWrite(westRed, HIGH);
  digitalWrite(westYellow, LOW);
  digitalWrite(westGreen, LOW);
}

void setTrafficLights(int north, int east, int south, int west) {
  // Coloca todas las luces para cada direccion
  // 0 = Rojo, 1 = Verde, 2 = Amarillo

  setDirectionLight(north, northRed, northYellow, northGreen);
  setDirectionLight(east, eastRed, eastYellow, eastGreen);
  setDirectionLight(south, southRed, southYellow, southGreen);
  setDirectionLight(west, westRed, westYellow, westGreen);
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