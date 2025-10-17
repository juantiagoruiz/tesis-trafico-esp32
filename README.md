# Control Adaptativo de Tráfico Urbano en San Luis Utilizando Nodos con ESP32-CAM
Este repositorio contiene todos los archivos, códigos fuente y documentación del proyecto final de carrera "Control Adaptativo de Tráfico Urbano en San Luis Utilizando Nodos con ESP32-CAM", presentado para obtener el título de Ingeniero Electrónico en la Universidad Nacional de San Luis.

El proyecto desarrolla un sistema de control de tráfico adaptativo de bajo costo, basado en visión artificial con TinyML y computación de borde (Edge Computing) sobre microcontroladores ESP32.

## Estructura del Repositorio
Los archivos del proyecto están organizados en las siguientes carpetas principales:

### /Tesis/
Contiene el documento final del proyecto de tesis en formato PDF. Este archivo describe en detalle el marco teórico, el diseño del sistema, las simulaciones, los resultados y las conclusiones.

### /SUMO/
Incluye todos los archivos necesarios para replicar la simulación microscópica de tráfico realizada con el software SUMO.

Mapas y Configuración: Archivos .xml y .sumocfg que definen el escenario vial de la ciudad de San Luis utilizado.

Scripts de Control: Scripts de Python que interactúan con SUMO a través de la interfaz TraCI para implementar y evaluar los diferentes algoritmos de control de semáforos (ATF, AACA, ACAB y ACAB Realista).

### /Firmware/
Contiene el código fuente completo para los microcontroladores ESP32, desarrollado para el IDE de Arduino.

/nodo_control_2vias/: Firmware para el ESP32 que actúa como controlador de la intersección. Implementa la máquina de estados para la gestión de los semáforos y aloja el servidor web para la visualización de datos. Realizado para dos intersecciones.

/nodo_control_2vias/data/: Contiene los archivos index.html, style.css y heatmap.min.js del servidor web. Estos archivos deben ser cargados en el sistema de archivos LittleFS del ESP32.

/nodo_control_4vias_wokwi/: Firmware para el ESP32 que actúa como controlador de la intersección. Implementa la máquina de estados para la gestión de los semáforos. Realizado para cuatro intersecciones.

/Unidad_Detectora_code/: Firmware para los nodos ESP32-S3-CAM. Este código se encarga de capturar imágenes, ejecutar el modelo de TinyML para la detección de vehículos y comunicar los resultados al Nodo de Control vía ESP-NOW.

/nodo_repetidor/: Firmware para los nodos ESP32 que se encargan de retransmitir mensajes ESP-NOW recibidos para ampliar rango de cobertura.


### /Modelo_TinyML/
Archivos relacionados con el modelo de aprendizaje automático.

/ei-vehicle-presence-arduino-1.0.15.zip/: Contiene la librería .zip exportada desde la plataforma Edge Impulse, lista para ser importada en el IDE de Arduino e integrada en el firmware de la Unidad Detectora.


### /Resultados_y_Anexos/

Material adicional y datos generados durante la investigación.

/Resultados de Simulacion/: Archivos .csv con las métricas obtenidas de las corridas en SUMO y los scripts de python utilizados para la generación de gráficas.

/Videos de Pruebas/: Enlaces a archivos de video de las pruebas de campo y del prototipo funcional.

## Tecnologías Utilizadas

Hardware: ESP32-S3-CAM, ESP32 DevKit

Software y Plataformas: SUMO, Arduino IDE, Edge Impulse, Python

Protocolos y Librerías: ESP-NOW, TraCI, LittleFS, C++
